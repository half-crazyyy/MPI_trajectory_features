#include "benchmark.hpp"
#include "csv_reader.hpp"
#include "distributed_processor.hpp"

#include <mpi.h>
#include <fmt/core.h>
#include <exception>
#include <iomanip>
#include <iostream>

int main(int argc, char** argv) {
    // Инициализация MPI-среды
    MPI_Init(&argc, &argv);

    int rank = 0;
    int process_count = 1;

    // Процесс с rank = 0 используется как главный - он выводит результаты,
    // сохраняет файлы и выполняет проверку корректности.
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &process_count);

    int exit_code = 0;

    try {
        // Разбор аргументов командной строки:
        // входной CSV, выходной CSV, число повторов, режим проверки и т.д.
        const auto options = parse_arguments(argc, argv);

        // Справка выводится только главным процессом, чтобы при запуске
        // через mpiexec один и тот же текст не печатался несколько раз.
        if (options.help) {
            if (rank == 0) {
                print_help(argv[0]);
            }
            MPI_Finalize();
            return 0;
        }

        if (rank == 0 && options.generate_sample) {
            write_sample_dataset(options.input_path, 2000, 80);
            std::cout << "Sample dataset generated: " << options.input_path << '\n';
        }

        // Барьер синхронизации нужен, чтобы все процессы дождались окончания генерации файла перед началом распределённой обработки
        MPI_Barrier(MPI_COMM_WORLD);

        // Основной этап программы
        const auto result = run_distributed_processing(options.input_path, options.repetitions);

        // Итоговые файлы и сводная информация формируются именно на rank = 0
        if (rank == 0) {
            write_feature_csv(options.output_path, result.final_stats);

            fmt::print("MPI trajectory feature calculation finished\n");
            fmt::print("Processes: {}\n", process_count);
            fmt::print("Input points: {}\n", result.total_points);
            fmt::print("Trajectories: {}\n", result.total_trajectories);
            fmt::print("Best time: {:.6f} seconds\n", result.elapsed_seconds);
            fmt::print("Output: {}\n", options.output_path);


            // Сохранение строки бенчмарка в общий CSV-файл
            // По этому файлу затем строятся таблицы ускорения и графики
            append_benchmark_row("docs/performance/benchmark_results.csv",
                                 process_count,
                                 result.total_points,
                                 result.total_trajectories,
                                 result.elapsed_seconds);

            if (options.verify) {
                const auto reference = run_sequential_reference(options.input_path);
                const bool ok = compare_results(reference, result.final_stats);
                std::cout << "Verification: " << (ok ? "OK" : "FAILED") << '\n';
                if (!ok) {
                    exit_code = 2;
                }
            }
        }
    } catch (const std::exception& e) {
        // Ошибку выводит только главный процесс, чтобы сообщение не дублировалось от каждого MPI-процесса
        if (rank == 0) {
            std::cerr << "ERROR: " << e.what() << '\n';
        }
        exit_code = 1;
    }

    // Завершение MPI-среды для всех процессов
    MPI_Finalize();
    return exit_code;
}
