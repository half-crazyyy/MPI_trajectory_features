#!/usr/bin/env python3
"""Generate synthetic air-object trajectory dataset for MPI benchmark."""

import argparse
import csv
import math
import random
from pathlib import Path

CLASSES = ["plane", "drone", "helicopter", "balloon"]


def generate(path: Path, trajectories_per_class: int, points_per_trajectory: int) -> None:
    random.seed(42)
    path.parent.mkdir(parents=True, exist_ok=True)

    with path.open("w", newline="", encoding="utf-8") as file:
        writer = csv.writer(file)
        writer.writerow(["category", "record_index", "element_index", "v", "h"])

        for category_index, category in enumerate(CLASSES):
            for record_index in range(trajectories_per_class):
                base_v = 35.0 + category_index * 45.0 + (record_index % 7) * 1.5
                base_h = 200.0 + category_index * 900.0 + (record_index % 11) * 10.0

                for element_index in range(points_per_trajectory):
                    t = element_index / max(1, points_per_trajectory - 1)
                    velocity = base_v + 8.0 * math.sin(2 * math.pi * t) + random.gauss(0.0, 1.0)
                    height = base_h + 120.0 * t + 25.0 * math.cos(2 * math.pi * t) + random.gauss(0.0, 4.0)
                    writer.writerow([category, record_index, element_index, f"{velocity:.6f}", f"{height:.6f}"])


def main() -> None:
    parser = argparse.ArgumentParser(description="Generate synthetic trajectory dataset")
    parser.add_argument("--output", default="data/sample_air_objects.csv")
    parser.add_argument("--trajectories-per-class", type=int, default=2000)
    parser.add_argument("--points-per-trajectory", type=int, default=80)
    args = parser.parse_args()

    generate(Path(args.output), args.trajectories_per_class, args.points_per_trajectory)
    print(f"Dataset written to {args.output}")


if __name__ == "__main__":
    main()
