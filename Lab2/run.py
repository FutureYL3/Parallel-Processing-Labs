#!/usr/bin/env python3
"""Run matrix transpose measurements using this experiment's local build."""
import argparse
import csv
from datetime import datetime, timezone
import importlib.util
import io
import json
import os
from pathlib import Path
import platform
import random
import statistics
import subprocess
import sys

HERE = Path(__file__).resolve().parent
DEFAULT_SIZES = [2048, 3072, 4096, 6144, 8192]
DEFAULT_TILES = [16, 32, 64, 128]


def save(rows, destination):
    with destination.open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=list(rows[0]))
        writer.writeheader(); writer.writerows(rows)


def environment():
    info = {"recorded_at_utc": datetime.now(timezone.utc).isoformat(),
            "system": platform.system(), "architecture": platform.machine(),
            "logical_cpus": os.cpu_count(), "python": platform.python_version(),
            "openmp_environment": {k: v for k, v in os.environ.items() if k.startswith(("OMP_", "KMP_"))}}
    if platform.system() == "Darwin":
        for key in ["machdep.cpu.brand_string", "hw.perflevel0.physicalcpu", "hw.perflevel1.physicalcpu"]:
            info[key] = subprocess.run(["sysctl", "-n", key], text=True, capture_output=True).stdout.strip()
    cache = HERE / "build/CMakeCache.txt"
    info["compiler"] = next(line.split("=", 1)[1] for line in cache.read_text().splitlines()
                            if line.startswith("CMAKE_CXX_COMPILER:"))
    info["compiler_version"] = subprocess.check_output([info["compiler"], "--version"], text=True).splitlines()[0]
    return info


def run_benchmark(output, threads=4, sizes=None, tiles=None, repeat=7, rounds=3, seed=2026):
    sizes = sorted(set(DEFAULT_SIZES if sizes is None else sizes))
    tiles = sorted(set(DEFAULT_TILES if tiles is None else tiles))
    output = Path(output)
    output.mkdir(parents=True, exist_ok=True)
    counts = sorted({1, threads} | ({2} if threads >= 2 else set()))
    variants = [("Naive", "naive", tiles[0])] + [(f"Tile{tile}", "tiled", tile) for tile in tiles]
    cases = [(label, kernel, tile, workers) for workers in counts for label, kernel, tile in variants]
    metadata = environment()
    metadata.update({"matrix_sizes": sizes, "tile_sizes": tiles, "repeats": repeat, "rounds": rounds,
                     "warmups_per_process": 1, "requested_threads": counts, "seed": seed,
                     "golden_ratio_commit": "fb012f772374bf826f81582a31e95455c91fcb26"})
    (output / "environment.json").write_text(json.dumps(metadata, indent=2, ensure_ascii=False) + "\n")
    raw = []
    randomizer = random.Random(seed)

    def measure(n, outer, label, kernel, tile, workers):
        options = {"n": n, "tile": tile, "seed": seed, "kernel": kernel,
                   "threads": workers, "repeat": repeat}
        command = [str(HERE / "build/lab2_locality")]
        for name, value in options.items():
            command.extend(["--" + name, str(value)])
        print(f"Lab2 round {outer + 1}/{rounds}: N={n}, {label}, threads={workers}",
              file=sys.stderr, flush=True)
        output_text = subprocess.check_output(command, text=True)
        for row in csv.DictReader(io.StringIO(output_text)):
            row.update(round=outer, variant=label, requested_threads=workers)
            raw.append(row)

    for outer in range(rounds):
        order_sizes = sizes.copy(); randomizer.shuffle(order_sizes)
        for n in order_sizes:
            measure(n, outer, "Serial", "serial", tiles[0], 1)
            order = cases.copy(); randomizer.shuffle(order)
            for label, kernel, tile, workers in order:
                measure(n, outer, label, kernel, tile, workers)
            measure(n, outer, "Serial", "serial", tiles[0], 1)
            save(raw, output / "raw.csv")

    summaries = [row for row in raw if row["record"] == "summary"]
    configurations = [("Serial", 1)] + [(label, workers) for workers in counts for label, _, _ in variants]
    medians = {}
    for n in sizes:
        for label, workers in configurations:
            selected = [row for row in summaries if int(row["n"]) == n
                        and row["variant"] == label and int(row["threads"]) == workers]
            medians[n, label, workers] = statistics.median(float(row["elapsed_s"]) for row in selected)
    results = []
    for n in sizes:
        for label, workers in configurations:
            selected = [row for row in summaries if int(row["n"]) == n
                        and row["variant"] == label and int(row["threads"]) == workers]
            trials = [float(row["elapsed_s"]) for row in raw if row["record"] == "trial"
                      and int(row["n"]) == n and row["variant"] == label and int(row["threads"]) == workers]
            elapsed = medians[n, label, workers]
            speed = medians[n, "Serial", 1] / elapsed
            thread_speed = medians[n, label, 1] / elapsed
            row = min(selected, key=lambda item: abs(float(item["elapsed_s"]) - elapsed)).copy()
            row.update(elapsed_s=elapsed, median_ms=elapsed * 1000,
                       effective_gbps=2.0 * n * n * 4 / elapsed / 1e9,
                       speedup=speed, efficiency=speed / workers,
                       thread_speedup=thread_speed, thread_efficiency=thread_speed / workers,
                       round_min_ms=min(float(item["elapsed_s"]) for item in selected) * 1000,
                       round_max_ms=max(float(item["elapsed_s"]) for item in selected) * 1000,
                       samples=len(trials), trial_min_ms=min(trials) * 1000, trial_max_ms=max(trials) * 1000)
            results.append(row)
    save(results, output / "summary.csv")
    spec = importlib.util.spec_from_file_location("lab2_plot", HERE / "plot.py")
    module = importlib.util.module_from_spec(spec); spec.loader.exec_module(module)
    module.plot_results(output / "summary.csv", output, threads)
    print(f"Lab2 measurements and figures: {output}", file=sys.stderr)
    return results


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--n", type=int, nargs="+", default=DEFAULT_SIZES)
    parser.add_argument("--tile", type=int, nargs="+", default=DEFAULT_TILES)
    parser.add_argument("--threads", type=int, default=4)
    parser.add_argument("--repeat", type=int, default=7)
    parser.add_argument("--rounds", type=int, default=3)
    parser.add_argument("--seed", type=int, default=2026)
    parser.add_argument("--output", type=Path, default=HERE / "results")
    args = parser.parse_args()
    run_benchmark(args.output, threads=args.threads, sizes=args.n, tiles=args.tile,
                  repeat=args.repeat, rounds=args.rounds, seed=args.seed)


if __name__ == "__main__":
    main()
