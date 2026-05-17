#include "benchmark.hpp"
#include "csv_reader.hpp"

#include <fstream>
#include <iostream>
#include <stdexcept>

ProgramOptions parse_arguments(int argc, char** argv) {
    ProgramOptions options;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            options.help = true;
        } else if (arg == "--input" && i + 1 < argc) {
            options.input_path = argv[++i];
        } else if (arg == "--output" && i + 1 < argc) {
            options.output_path = argv[++i];
        } else if (arg == "--repetitions" && i + 1 < argc) {
            options.repetitions = std::stoi(argv[++i]);
        } else if (arg == "--generate-sample") {
            options.generate_sample = true;
        } else if (arg == "--verify") {
            options.verify = true;
        } else {
            throw std::runtime_error("Unknown or incomplete argument: " + arg);
        }
    }

    return options;
}

void print_help(const char* executable_name) {
    std::cout
        << "Usage:\n"
        << "  mpiexec -n 4 " << executable_name
        << " --input data/sample_air_objects.csv --output features.csv --verify\n\n"
        << "Options:\n"
        << "  --input FILE          Input CSV file\n"
        << "  --output FILE         Output feature CSV file\n"
        << "  --repetitions N       Number of benchmark repetitions, best time is used\n"
        << "  --generate-sample     Generate sample dataset before processing\n"
        << "  --verify              Compare MPI result with sequential reference\n"
        << "  --help                Show this help message\n";
}

std::vector<FinalTrajectoryStats> run_sequential_reference(const std::string& input_path) {
    const auto points = read_csv_points(input_path);
    const auto partial = calculate_local_stats(points);
    return finalize_stats(partial);
}

void append_benchmark_row(const std::string& path,
                          int process_count,
                          int points,
                          int trajectories,
                          double elapsed_seconds) {
    bool need_header = false;
    {
        std::ifstream check(path);
        need_header = !check.good() || check.peek() == std::ifstream::traits_type::eof();
    }

    std::ofstream out(path, std::ios::app);
    if (!out.is_open()) {
        throw std::runtime_error("Cannot open benchmark file: " + path);
    }

    if (need_header) {
        out << "processes,points,trajectories,time_seconds\n";
    }
    out << process_count << ',' << points << ',' << trajectories << ',' << elapsed_seconds << '\n';
}
