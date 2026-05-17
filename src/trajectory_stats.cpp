#include "trajectory_stats.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <stdexcept>

PartialStatsMap calculate_local_stats(const std::vector<TrajectoryPoint>& points) {
    PartialStatsMap result;

    for (const auto& point : points) {
        TrajectoryKey key{point.category, point.record_index};
        auto it = result.find(key);

        if (it == result.end()) {
            PartialTrajectoryStats stats;
            stats.category = point.category;
            stats.record_index = point.record_index;
            stats.point_count = 1;
            stats.hmin = point.height;
            stats.hmax = point.height;
            stats.hsum = point.height;
            stats.vmin = point.velocity;
            stats.vmax = point.velocity;
            stats.vsum = point.velocity;
            result[key] = stats;
        } else {
            auto& stats = it->second;
            ++stats.point_count;
            stats.hmin = std::min(stats.hmin, point.height);
            stats.hmax = std::max(stats.hmax, point.height);
            stats.hsum += point.height;
            stats.vmin = std::min(stats.vmin, point.velocity);
            stats.vmax = std::max(stats.vmax, point.velocity);
            stats.vsum += point.velocity;
        }
    }

    return result;
}

void merge_partial_stat(PartialStatsMap& target, const PartialTrajectoryStats& source) {
    TrajectoryKey key{source.category, source.record_index};
    auto it = target.find(key);

    if (it == target.end()) {
        target[key] = source;
        return;
    }

    // При объединении частичных результатов среднее не усредняется напрямую
    // Сначала складываются суммы и количество точек, а среднее считается в конце
    auto& current = it->second;
    current.point_count += source.point_count;
    current.hmin = std::min(current.hmin, source.hmin);
    current.hmax = std::max(current.hmax, source.hmax);
    current.hsum += source.hsum;
    current.vmin = std::min(current.vmin, source.vmin);
    current.vmax = std::max(current.vmax, source.vmax);
    current.vsum += source.vsum;
}

std::vector<PartialTrajectoryStats> map_to_vector(const PartialStatsMap& stats) {
    std::vector<PartialTrajectoryStats> result;
    result.reserve(stats.size());
    for (const auto& entry : stats) {
        result.push_back(entry.second);
    }
    return result;
}

std::vector<FinalTrajectoryStats> finalize_stats(const PartialStatsMap& stats) {
    std::vector<FinalTrajectoryStats> result;
    result.reserve(stats.size());

    for (const auto& entry : stats) {
        const auto& partial = entry.second;
        if (partial.point_count <= 0) {
            continue;
        }

        FinalTrajectoryStats final_stats;
        final_stats.category = partial.category;
        final_stats.record_index = partial.record_index;
        final_stats.point_count = partial.point_count;
        final_stats.hmin = partial.hmin;
        final_stats.hmax = partial.hmax;
        final_stats.haverage = partial.hsum / static_cast<double>(partial.point_count);
        final_stats.vmin = partial.vmin;
        final_stats.vmax = partial.vmax;
        final_stats.vaverage = partial.vsum / static_cast<double>(partial.point_count);
        final_stats.hrange = final_stats.hmax - final_stats.hmin;
        final_stats.vrange = final_stats.vmax - final_stats.vmin;
        result.push_back(final_stats);
    }

    return result;
}

void write_feature_csv(const std::string& path, const std::vector<FinalTrajectoryStats>& stats) {
    std::ofstream out(path);
    if (!out.is_open()) {
        throw std::runtime_error("Cannot create output CSV: " + path);
    }

    out << "category,record_index,point_count,hmin,hmax,haverage,vmin,vmax,vaverage,hrange,vrange\n";
    for (const auto& row : stats) {
        out << category_name(row.category) << ','
            << row.record_index << ','
            << row.point_count << ','
            << row.hmin << ','
            << row.hmax << ','
            << row.haverage << ','
            << row.vmin << ','
            << row.vmax << ','
            << row.vaverage << ','
            << row.hrange << ','
            << row.vrange << '\n';
    }
}

bool compare_results(const std::vector<FinalTrajectoryStats>& left,
                     const std::vector<FinalTrajectoryStats>& right,
                     double eps) {
    if (left.size() != right.size()) {
        return false;
    }

    auto close = [eps](double a, double b) {
        return std::fabs(a - b) <= eps;
    };

    for (std::size_t i = 0; i < left.size(); ++i) {
        const auto& a = left[i];
        const auto& b = right[i];
        if (a.category != b.category || a.record_index != b.record_index || a.point_count != b.point_count) {
            return false;
        }
        if (!close(a.hmin, b.hmin) || !close(a.hmax, b.hmax) || !close(a.haverage, b.haverage)) {
            return false;
        }
        if (!close(a.vmin, b.vmin) || !close(a.vmax, b.vmax) || !close(a.vaverage, b.vaverage)) {
            return false;
        }
    }

    return true;
}
