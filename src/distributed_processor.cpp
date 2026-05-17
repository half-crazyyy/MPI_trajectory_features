#include "distributed_processor.hpp"
#include "csv_reader.hpp"

#include <mpi.h>
#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace {

std::vector<double> pack_points(const std::vector<TrajectoryPoint>& points) {
    std::vector<double> packed(points.size() * 5);
    for (std::size_t i = 0; i < points.size(); ++i) {
        packed[i * 5 + 0] = static_cast<double>(points[i].category);
        packed[i * 5 + 1] = static_cast<double>(points[i].record_index);
        packed[i * 5 + 2] = static_cast<double>(points[i].element_index);
        packed[i * 5 + 3] = points[i].velocity;
        packed[i * 5 + 4] = points[i].height;
    }
    return packed;
}

std::vector<TrajectoryPoint> unpack_points(const std::vector<double>& packed) {
    std::vector<TrajectoryPoint> points(packed.size() / 5);
    for (std::size_t i = 0; i < points.size(); ++i) {
        points[i].category = static_cast<int>(packed[i * 5 + 0]);
        points[i].record_index = static_cast<int>(packed[i * 5 + 1]);
        points[i].element_index = static_cast<int>(packed[i * 5 + 2]);
        points[i].velocity = packed[i * 5 + 3];
        points[i].height = packed[i * 5 + 4];
    }
    return points;
}

std::vector<int> build_counts(int total_points, int process_count) {
    std::vector<int> counts(process_count, 0);
    const int base = total_points / process_count;
    const int rem = total_points % process_count;

    for (int rank = 0; rank < process_count; ++rank) {
        counts[rank] = base + (rank < rem ? 1 : 0);
    }
    return counts;
}

std::vector<int> build_displacements(const std::vector<int>& counts) {
    std::vector<int> displs(counts.size(), 0);
    for (std::size_t i = 1; i < counts.size(); ++i) {
        displs[i] = displs[i - 1] + counts[i - 1];
    }
    return displs;
}

} // namespace

DistributedResult run_distributed_processing(const std::string& input_path, int repetitions) {
    int rank = 0;
    int process_count = 1;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &process_count);

    if (repetitions < 1) {
        repetitions = 1;
    }

    DistributedResult best_result;
    best_result.elapsed_seconds = 1e100;

    for (int rep = 0; rep < repetitions; ++rep) {
        std::vector<TrajectoryPoint> all_points;
        if (rank == 0) {
            all_points = read_csv_points(input_path);
        }

        int total_points = static_cast<int>(all_points.size());
        MPI_Bcast(&total_points, 1, MPI_INT, 0, MPI_COMM_WORLD);

        const auto point_counts = build_counts(total_points, process_count);
        const auto point_displs = build_displacements(point_counts);

        std::vector<int> send_counts(process_count, 0);
        std::vector<int> send_displs(process_count, 0);
        for (int i = 0; i < process_count; ++i) {
            send_counts[i] = point_counts[i] * 5;
            send_displs[i] = point_displs[i] * 5;
        }

        std::vector<double> packed_all;
        if (rank == 0) {
            packed_all = pack_points(all_points);
        }

        std::vector<double> packed_local(point_counts[rank] * 5);

        MPI_Barrier(MPI_COMM_WORLD);
        const double start_time = MPI_Wtime();

        // Главный процесс распределяет блоки строк между всеми MPI-процессами
        MPI_Scatterv(packed_all.data(),
                     send_counts.data(),
                     send_displs.data(),
                     MPI_DOUBLE,
                     packed_local.data(),
                     static_cast<int>(packed_local.size()),
                     MPI_DOUBLE,
                     0,
                     MPI_COMM_WORLD);

        const auto local_points = unpack_points(packed_local);
        const auto local_map = calculate_local_stats(local_points);
        const auto local_vector = map_to_vector(local_map);

        const int local_bytes = static_cast<int>(local_vector.size() * sizeof(PartialTrajectoryStats));
        std::vector<int> recv_bytes(process_count, 0);

        MPI_Gather(&local_bytes, 1, MPI_INT,
                   recv_bytes.data(), 1, MPI_INT,
                   0, MPI_COMM_WORLD);

        std::vector<int> recv_displs;
        std::vector<char> gathered_bytes;
        int total_bytes = 0;

        if (rank == 0) {
            recv_displs = build_displacements(recv_bytes);
            total_bytes = recv_displs.empty() ? 0 : recv_displs.back() + recv_bytes.back();
            gathered_bytes.resize(total_bytes);
        }

        MPI_Gatherv(reinterpret_cast<const char*>(local_vector.data()),
                    local_bytes,
                    MPI_BYTE,
                    gathered_bytes.data(),
                    recv_bytes.data(),
                    recv_displs.data(),
                    MPI_BYTE,
                    0,
                    MPI_COMM_WORLD);

        MPI_Barrier(MPI_COMM_WORLD);
        const double elapsed = MPI_Wtime() - start_time;

        if (rank == 0) {
            PartialStatsMap merged;
            int offset = 0;
            for (int bytes : recv_bytes) {
                const int count = bytes / static_cast<int>(sizeof(PartialTrajectoryStats));
                for (int i = 0; i < count; ++i) {
                    PartialTrajectoryStats stat;
                    std::memcpy(&stat,
                                gathered_bytes.data() + offset + i * sizeof(PartialTrajectoryStats),
                                sizeof(PartialTrajectoryStats));
                    merge_partial_stat(merged, stat);
                }
                offset += bytes;
            }

            const auto final_stats = finalize_stats(merged);
            if (elapsed < best_result.elapsed_seconds) {
                best_result.final_stats = final_stats;
                best_result.elapsed_seconds = elapsed;
                best_result.total_points = total_points;
                best_result.total_trajectories = static_cast<int>(final_stats.size());
            }
        }
    }

    return best_result;
}
