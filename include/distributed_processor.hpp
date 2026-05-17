#ifndef DISTRIBUTED_PROCESSOR_HPP
#define DISTRIBUTED_PROCESSOR_HPP

#include "trajectory_stats.hpp"
#include <string>
#include <vector>

struct DistributedResult {
    std::vector<FinalTrajectoryStats> final_stats;
    double elapsed_seconds = 0.0;
    int total_points = 0;
    int total_trajectories = 0;
};

// Выполняет полный MPI-расчёт: чтение на rank 0, раздача точек,
// локальный расчёт статистик, сбор и объединение результата
DistributedResult run_distributed_processing(const std::string& input_path, int repetitions);

#endif // DISTRIBUTED_PROCESSOR_HPP
