#!/usr/bin/env bash
set -euo pipefail

EXECUTABLE="${1:-./build/mpi_trajectory_features}"
INPUT="${2:-data/sample_air_objects.csv}"
MAX_PROCESSES="${3:-12}"
OUTPUT_DIR="docs/performance"
RESULTS="$OUTPUT_DIR/benchmark_results.csv"

# На локальных компьютерах OpenMPI ограничивает количество процессов числом доступных slots.
# Если нужно запустить больше MPI-процессов, чем физических ядер/slots, используется oversubscribe.
# Это полезно для учебного бенчмарка на 1..12 процессах на ноутбуке.
MPIEXEC_BIN="${MPIEXEC_BIN:-mpiexec}"
# Если переменная MPI_EXTRA_ARGS не задана, для OpenMPI автоматически включается oversubscribe.
# Если нужно запустить без дополнительных параметров, задайте MPI_EXTRA_ARGS="" перед запуском скрипта.
if [[ ${MPI_EXTRA_ARGS+x} ]]; then
    MPI_EXTRA_ARGS_VALUE="$MPI_EXTRA_ARGS"
else
    MPI_EXTRA_ARGS_VALUE=""
    if "$MPIEXEC_BIN" --version 2>/dev/null | grep -qi "Open MPI"; then
        MPI_EXTRA_ARGS_VALUE="--oversubscribe"
    fi
fi

mkdir -p "$OUTPUT_DIR"
rm -f "$RESULTS" "$OUTPUT_DIR/benchmark_summary.csv"
rm -f "$OUTPUT_DIR"/features_*.csv

for N in $(seq 1 "$MAX_PROCESSES"); do
    echo "Запуск бенчмарка на $N MPI-процесс(ах)"
    # shellcheck disable=SC2086
    "$MPIEXEC_BIN" $MPI_EXTRA_ARGS_VALUE -n "$N" "$EXECUTABLE" \
        --input "$INPUT" \
        --output "$OUTPUT_DIR/features_${N}.csv" \
        --repetitions 3 \
        --verify
    echo
done

python3 scripts/plot_results.py --input "$RESULTS" --output-dir "$OUTPUT_DIR"
