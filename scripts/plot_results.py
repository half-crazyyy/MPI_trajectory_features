#!/usr/bin/env python3
"""Построение графиков времени и ускорения по результатам MPI-бенчмарка."""

import argparse
import csv
from pathlib import Path

import matplotlib.pyplot as plt

# Явно задаём шрифт с поддержкой кириллицы для русских подписей графиков.
plt.rcParams["font.family"] = "DejaVu Sans"
plt.rcParams["axes.unicode_minus"] = False


def read_results(path: Path):
    rows = []
    with path.open("r", encoding="utf-8") as file:
        for row in csv.DictReader(file):
            rows.append({
                "processes": int(row["processes"]),
                "time_seconds": float(row["time_seconds"]),
            })
    rows.sort(key=lambda item: item["processes"])
    return rows


def main() -> None:
    parser = argparse.ArgumentParser(description="Построение графиков MPI-бенчмарка")
    parser.add_argument("--input", default="docs/performance/benchmark_results.csv")
    parser.add_argument("--output-dir", default="docs/performance")
    args = parser.parse_args()

    rows = read_results(Path(args.input))
    if not rows:
        return

    output_dir = Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    processes = [row["processes"] for row in rows]
    times = [row["time_seconds"] for row in rows]
    baseline = times[0]
    speedups = [baseline / value for value in times]
    efficiencies = [speedup / proc for speedup, proc in zip(speedups, processes)]
    ideal_speedups = processes[:]  # Идеальное линейное ускорение: S(p) = p.

    with (output_dir / "benchmark_summary.csv").open("w", encoding="utf-8") as file:
        file.write("processes,time_seconds,speedup,efficiency\n")
        for proc, time, speedup, efficiency in zip(processes, times, speedups, efficiencies):
            file.write(f"{proc},{time:.6f},{speedup:.6f},{efficiency:.6f}\n")

    plt.figure()
    plt.plot(processes, times, marker="o")
    plt.xlabel("Количество MPI-процессов")
    plt.ylabel("Время выполнения, с")
    plt.title("Зависимость времени выполнения от числа процессов")
    plt.xticks(processes)
    plt.grid(True)
    plt.savefig(output_dir / "time.png", dpi=160, bbox_inches="tight")
    plt.close()

    plt.figure()
    plt.plot(processes, speedups, marker="o", label="Фактическое ускорение")
    plt.plot(processes, ideal_speedups, linestyle="--", label="Идеальное линейное ускорение")
    plt.xlabel("Количество MPI-процессов")
    plt.ylabel("Ускорение")
    plt.title("Ускорение MPI-расчёта траекторных признаков")
    plt.xticks(processes)
    plt.grid(True)
    plt.legend()
    plt.savefig(output_dir / "speedup.png", dpi=160, bbox_inches="tight")
    plt.close()


if __name__ == "__main__":
    main()
