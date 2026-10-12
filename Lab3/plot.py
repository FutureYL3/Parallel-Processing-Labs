#!/usr/bin/env python3
"""Export histogram implementation, thread-count and lock-count comparisons."""
import argparse
import csv
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "third_party/golden_ratio"))
from golden_ratio_plot.config import PlotConfig
from golden_ratio_plot.reader import read_csv
from golden_ratio_plot.renderer.ablation import AblationRenderer


def size_label(n):
    return f"N={n / 1000000:g}M"


def write_csv(path, fields, rows):
    with path.open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=fields)
        writer.writeheader(); writer.writerows(rows)


def render(source, output, two_level=False, show_values=False):
    config = PlotConfig(input=str(source), output=str(output), mode="ablation",
                        width_pt=540, font_size_pt=7, y_min=0, show_values=show_values,
                        two_level_xaxis=two_level, formats=["pdf", "png", "svg"])
    AblationRenderer(config).render(read_csv(source))


def plot_results(summary_path, output, workers):
    output = Path(output)
    output.mkdir(parents=True, exist_ok=True)
    with Path(summary_path).open() as stream:
        rows = list(csv.DictReader(stream))
    sizes = sorted({int(row["n"]) for row in rows})
    distributions = [profile for profile in ("uniform", "hotspot") if any(row["distribution"] == profile for row in rows)]
    hot_percent = rows[0]["hot_percent"]
    profile_labels = {"uniform": "Uniform", "hotspot": f"Hot{hot_percent}%"}
    lock_counts = sorted({int(row["stripes"]) for row in rows if row["kernel"] == "locks"})
    variants = ["Critical", "Atomic"] + [f"Lock{count}" for count in lock_counts] + ["Private"]

    def find(n, distribution, label, count):
        return next(row for row in rows if int(row["n"]) == n and row["distribution"] == distribution
                    and row["variant"] == label and int(row["threads"]) == count)

    comparison = []
    lock_comparison = []
    scaling = []
    counts = sorted({int(row["threads"]) for row in rows if int(row["threads"]) > 1})
    for n in sizes:
        for distribution in distributions:
            major, minor = size_label(n), profile_labels[distribution]
            group = f"{n}-{distribution}"
            comparison.append({"group": group, "label": "Serial", "major_group": major,
                               "minor_group": minor, "Speedup over serial (×)": "1"})
            for label in variants:
                row = find(n, distribution, label, workers)
                comparison.append({"group": group, "label": label, "major_group": major,
                                   "minor_group": minor, "Speedup over serial (×)": f"{float(row['speedup']):.6g}"})
            for lock_count in lock_counts:
                row = find(n, distribution, f"Lock{lock_count}", workers)
                lock_comparison.append({"group": group, "label": f"Lock{lock_count}", "major_group": major,
                                        "minor_group": minor, "Lock-count speedup (×)": f"{float(row['lock_count_speedup']):.3g}"})
            for count in counts:
                for label in variants:
                    row = find(n, distribution, label, count)
                    scaling.append({"group": f"{group}-{count}", "label": label,
                                    "major_group": f"{major} {minor}", "minor_group": f"{count}T",
                                    "Thread speedup (×)": f"{float(row['thread_speedup']):.6g}"})
    comparison.append({"group": "__caption__", "label": "", "major_group": "", "minor_group": "",
                       "Speedup over serial (×)": f"{workers}-thread histogram, relative to the matched serial runtime"})
    source = output / "implementation_comparison.csv"
    write_csv(source, ["group", "label", "major_group", "minor_group", "Speedup over serial (×)"], comparison)
    render(source, output / "implementation_comparison", two_level=True)

    if scaling:
        scaling.append({"group": "__caption__", "label": "", "major_group": "", "minor_group": "",
                        "Thread speedup (×)": "T = threads; each kernel and lock count normalized to its one-thread runtime"})
        source = output / "thread_scaling.csv"
        write_csv(source, ["group", "label", "major_group", "minor_group", "Thread speedup (×)"], scaling)
        render(source, output / "thread_scaling", two_level=True)

    lock_comparison.append({"group": "__caption__", "label": "", "major_group": "", "minor_group": "",
                            "Lock-count speedup (×)": f"{workers} threads; relative to {lock_counts[0]} lock(s) on the matched input"})
    source = output / "lock_count_comparison.csv"
    write_csv(source, ["group", "label", "major_group", "minor_group", "Lock-count speedup (×)"], lock_comparison)
    render(source, output / "lock_count_comparison", two_level=True, show_values=True)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--threads", type=int, default=4)
    args = parser.parse_args()
    plot_results(args.input, args.output, args.threads)
