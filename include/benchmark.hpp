#ifndef BENCHMARK_HPP
#define BENCHMARK_HPP

#include "trajectory_stats.hpp"
#include <string>
#include <vector>

struct ProgramOptions {
    std::string input_path = "data/sample_air_objects.csv";
    std::string output_path = "features.csv";
    int repetitions = 1;
    bool generate_sample = false;
    bool verify = false;
    bool help = false;
};

ProgramOptions parse_arguments(int argc, char** argv);
void print_help(const char* executable_name);
std::vector<FinalTrajectoryStats> run_sequential_reference(const std::string& input_path);
void append_benchmark_row(const std::string& path,
                          int process_count,
                          int points,
                          int trajectories,
                          double elapsed_seconds);

#endif // BENCHMARK_HPP
