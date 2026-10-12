#!/usr/bin/env python3
"""Run CSR SpMM measurements using this experiment's local build."""
import argparse
import csv
from datetime import datetime, timezone
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

VARIANTS = [
    ("Rows", "rows", "static", 0),
    ("Dynamic", "rows", "dynamic", 1),
    ("NNZ blocks", "balanced", "static", 0),
    ("Tiled", "tiled", "static", 0),
]


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


def run_benchmark(output, threads=4, m=4096, k=8192, n=1024,
                  nnz_per_row=32, tile=128, block_nnz=256, repeat=7, rounds=3, seed=2026):
    output = Path(output)
    output.mkdir(parents=True, exist_ok=True)
    counts = sorted({1, threads} | ({2} if threads >= 2 else set()))
    metadata = environment()
    metadata.update({"m": m, "k": k, "n": n, "nnz_per_row": nnz_per_row, "tile": tile,
                     "block_nnz": block_nnz, "repeats": repeat, "rounds": rounds, "warmups_per_process": 1,
                     "requested_threads": counts, "seed": seed,
                     "golden_ratio_commit": "fb012f772374bf826f81582a31e95455c91fcb26"})
    (output / "environment.json").write_text(json.dumps(metadata, indent=2, ensure_ascii=False) + "\n")
    raw = []
    cases = [(label, kernel, schedule, chunk, workers) for workers in counts
             for label, kernel, schedule, chunk in VARIANTS]
    randomizer = random.Random(seed)

    def measure(profile, outer, label, kernel, schedule, chunk, workers):
        options = {"m": m, "k": k, "n": n, "nnz-per-row": nnz_per_row, "seed": seed,
                   "profile": profile, "kernel": kernel, "threads": workers,
                   "schedule": schedule, "chunk": chunk, "tile": tile,
                   "block-nnz": block_nnz, "repeat": repeat}
        command = [str(HERE / "build/lab1_spmm")]
        for name, value in options.items():
            command.extend(["--" + name, str(value)])
        print(f"Lab1 round {outer + 1}/{rounds}: {profile}, {label}, threads={workers}", file=sys.stderr, flush=True)
        text = subprocess.check_output(command, text=True)
        for row in csv.DictReader(io.StringIO(text)):
            row.update(round=outer, variant=label, requested_threads=workers)
            raw.append(row)

    # Bracket each batch with a serial run; shuffle all other cases to spread drift.
    for outer in range(rounds):
        profiles = ["uniform", "skewed", "blocked"]
        randomizer.shuffle(profiles)
        for profile in profiles:
            measure(profile, outer, "Serial", "serial", "static", 0, 1)
            order = cases.copy(); randomizer.shuffle(order)
            for label, kernel, schedule, chunk, workers in order:
                measure(profile, outer, label, kernel, schedule, chunk, workers)
            measure(profile, outer, "Serial", "serial", "static", 0, 1)
            save(raw, output / "raw.csv")

    summaries = [r for r in raw if r["record"] == "summary"]
    medians = {}
    for profile in ["uniform", "skewed", "blocked"]:
        for label, workers in [("Serial", 1)] + [(v[0], p) for p in counts for v in VARIANTS]:
            rows = [r for r in summaries if r["profile"] == profile and r["variant"] == label and int(r["threads"]) == workers]
            medians[profile, label, workers] = statistics.median(float(r["elapsed_s"]) for r in rows)
    results = []
    for profile in ["uniform", "skewed", "blocked"]:
        for label, workers in [("Serial", 1)] + [(v[0], p) for p in counts for v in VARIANTS]:
            selected = [r for r in summaries if r["profile"] == profile and r["variant"] == label and int(r["threads"]) == workers]
            trials = [float(r["elapsed_s"]) for r in raw if r["record"] == "trial"
                      and r["profile"] == profile and r["variant"] == label and int(r["threads"]) == workers]
            elapsed = medians[profile, label, workers]
            speed = medians[profile, "Serial", 1] / elapsed
            thread_speed = medians[profile, label, 1] / elapsed
            row = min(selected, key=lambda r: abs(float(r["elapsed_s"]) - elapsed)).copy()
            row.update(elapsed_s=elapsed, median_ms=elapsed * 1000,
                       gflops=2.0 * float(row["nnz"]) * n / elapsed / 1e9,
                       partition_s=statistics.median(float(r["partition_s"]) for r in selected),
                       speedup=speed, efficiency=speed / workers,
                       thread_speedup=thread_speed, thread_efficiency=thread_speed / workers,
                       round_min_ms=min(float(r["elapsed_s"]) for r in selected) * 1000,
                       round_max_ms=max(float(r["elapsed_s"]) for r in selected) * 1000,
                       samples=len(trials), trial_min_ms=min(trials) * 1000, trial_max_ms=max(trials) * 1000)
            results.append(row)
    save(results, output / "summary.csv")
    # Load this experiment's renderer.
    import importlib.util
    spec = importlib.util.spec_from_file_location("lab1_plot", HERE / "plot.py")
    module = importlib.util.module_from_spec(spec); spec.loader.exec_module(module)
    module.plot_results(output / "summary.csv", output, threads)
    print(f"Lab1 measurements and figures: {output}", file=sys.stderr)
    return results


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--threads", type=int, default=4)
    parser.add_argument("--m", type=int, default=4096)
    parser.add_argument("--k", type=int, default=8192)
    parser.add_argument("--n", type=int, default=1024)
    parser.add_argument("--nnz-per-row", type=int, default=32)
    parser.add_argument("--tile", type=int, default=128)
    parser.add_argument("--block-nnz", type=int, default=256)
    parser.add_argument("--repeat", type=int, default=7)
    parser.add_argument("--rounds", type=int, default=3)
    parser.add_argument("--seed", type=int, default=2026)
    parser.add_argument("--output", type=Path, default=HERE / "results")
    args = parser.parse_args()
    run_benchmark(args.output, threads=args.threads, m=args.m, k=args.k, n=args.n,
                  nnz_per_row=args.nnz_per_row, tile=args.tile, block_nnz=args.block_nnz,
                  repeat=args.repeat, rounds=args.rounds, seed=args.seed)


if __name__ == "__main__":
    main()
