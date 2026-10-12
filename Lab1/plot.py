#!/usr/bin/env python3
"""Export implementation comparisons and thread scaling using the original renderer."""
import argparse
import csv
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "third_party/golden_ratio"))
from golden_ratio_plot.config import PlotConfig
from golden_ratio_plot.reader import read_csv
from golden_ratio_plot.renderer.ablation import AblationRenderer

VARIANTS = ["Rows", "Dynamic", "NNZ blocks", "Tiled"]
PROFILES = {"uniform": "Uniform", "skewed": "Skewed", "blocked": "Blocked"}


def write_csv(path, fields, rows):
    with path.open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=fields)
        writer.writeheader()
        writer.writerows(rows)


def render(source, output, two_level=False):
    config = PlotConfig(input=str(source), output=str(output), mode="ablation",
                        width_pt=540, font_size_pt=7, y_min=0,
                        show_values=True, two_level_xaxis=two_level,
                        formats=["pdf", "png", "svg"])
    AblationRenderer(config).render(read_csv(source))


def plot_results(summary_path, output, workers):
    output = Path(output)
    output.mkdir(parents=True, exist_ok=True)
    rows = list(csv.DictReader(Path(summary_path).open()))
    overview = []
    for profile, group in PROFILES.items():
        overview.append({"group": group, "label": "Serial", "Speedup over serial (×)": "1"})
        for label in VARIANTS:
            row = next(r for r in rows if r["profile"] == profile and r["variant"] == label and int(r["threads"]) == workers)
            overview.append({"group": group, "label": label, "Speedup over serial (×)": f"{float(row['speedup']):.2f}"})
    source = output / "implementation_comparison.csv"
    overview.append({"group": "__caption__", "label": "", "Speedup over serial (×)": f"{workers}-thread SpMM, relative to serial"})
    write_csv(source, ["group", "label", "Speedup over serial (×)"], overview)
    render(source, output / "implementation_comparison")

    scaling = []
    counts = sorted({int(r["threads"]) for r in rows if int(r["threads"]) > 1})
    for profile, group in PROFILES.items():
        for count in counts:
            for label in VARIANTS:
                row = next(r for r in rows if r["profile"] == profile and r["variant"] == label and int(r["threads"]) == count)
                scaling.append({"group": f"{group}-{count}", "label": label,
                                "major_group": group, "minor_group": f"{count} threads",
                                "Same-kernel thread speedup (×)": f"{float(row['thread_speedup']):.2f}"})
    if scaling:
        source = output / "thread_scaling.csv"
        scaling.append({"group": "__caption__", "label": "", "major_group": "", "minor_group": "",
                        "Same-kernel thread speedup (×)": "Each kernel normalized to its own one-thread runtime"})
        write_csv(source, ["group", "label", "major_group", "minor_group", "Same-kernel thread speedup (×)"], scaling)
        render(source, output / "thread_scaling", two_level=True)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--threads", type=int, default=4)
    args = parser.parse_args()
    plot_results(args.input, args.output, args.threads)
