#ifndef TRAJECTORY_STATS_HPP
#define TRAJECTORY_STATS_HPP

#include "csv_reader.hpp"
#include <map>
#include <string>
#include <vector>

// Ключ траектории: класс объекта + номер записи
struct TrajectoryKey {
    int category = 0;
    int record_index = 0;

    bool operator<(const TrajectoryKey& other) const {
        if (category != other.category) {
            return category < other.category;
        }
        return record_index < other.record_index;
    }
};

// Промежуточная статистика. Суммы нужны для корректного объединения
// частичных результатов с разных MPI-процессов
struct PartialTrajectoryStats {
    int category = 0;
    int record_index = 0;
    int point_count = 0;
    double hmin = 0.0;
    double hmax = 0.0;
    double hsum = 0.0;
    double vmin = 0.0;
    double vmax = 0.0;
    double vsum = 0.0;
};

// Итоговая статистика, которая сохраняется в выходной CSV
struct FinalTrajectoryStats {
    int category = 0;
    int record_index = 0;
    int point_count = 0;
    double hmin = 0.0;
    double hmax = 0.0;
    double haverage = 0.0;
    double vmin = 0.0;
    double vmax = 0.0;
    double vaverage = 0.0;
    double hrange = 0.0;
    double vrange = 0.0;
};

using PartialStatsMap = std::map<TrajectoryKey, PartialTrajectoryStats>;

PartialStatsMap calculate_local_stats(const std::vector<TrajectoryPoint>& points);
void merge_partial_stat(PartialStatsMap& target, const PartialTrajectoryStats& source);
std::vector<PartialTrajectoryStats> map_to_vector(const PartialStatsMap& stats);
std::vector<FinalTrajectoryStats> finalize_stats(const PartialStatsMap& stats);
void write_feature_csv(const std::string& path, const std::vector<FinalTrajectoryStats>& stats);
bool compare_results(const std::vector<FinalTrajectoryStats>& left,
                     const std::vector<FinalTrajectoryStats>& right,
                     double eps = 1e-9);

#endif // TRAJECTORY_STATS_HPP
