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


def write_csv(path, fields, rows):
    with path.open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=fields)
        writer.writeheader(); writer.writerows(rows)


def render(source, output, two_level=False):
    config = PlotConfig(input=str(source), output=str(output), mode="ablation",
                        width_pt=540, font_size_pt=7, y_min=0, show_values=True,
                        two_level_xaxis=two_level, formats=["pdf", "png", "svg"])
    AblationRenderer(config).render(read_csv(source))


def plot_results(summary_path, output, workers):
    output = Path(output)
    output.mkdir(parents=True, exist_ok=True)
    with Path(summary_path).open() as stream:
        rows = list(csv.DictReader(stream))
    sizes = sorted({int(row["n"]) for row in rows})
    tiles = sorted({int(row["tile"]) for row in rows if row["kernel"] == "tiled"})
    variants = ["Naive"] + [f"Tile{tile}" for tile in tiles]
    comparisons = []
    for n in sizes:
        group = f"N={n}"
        comparisons.append({"group": group, "label": "Serial", "Speedup over serial (×)": "1"})
        for label in variants:
            row = next(item for item in rows if int(item["n"]) == n
                       and item["variant"] == label and int(item["threads"]) == workers)
            comparisons.append({"group": group, "label": label,
                                "Speedup over serial (×)": f"{float(row['speedup']):.2f}"})
    comparisons.append({"group": "__caption__", "label": "",
                        "Speedup over serial (×)": f"{workers}-thread transpose, relative to the matched serial runtime"})
    source = output / "implementation_comparison.csv"
    write_csv(source, ["group", "label", "Speedup over serial (×)"], comparisons)
    render(source, output / "implementation_comparison")

    scaling = []
    counts = sorted({int(row["threads"]) for row in rows if int(row["threads"]) > 1})
    for n in sizes:
        for count in counts:
            for label in variants:
                row = next(item for item in rows if int(item["n"]) == n
                           and item["variant"] == label and int(item["threads"]) == count)
                scaling.append({"group": f"N{n}-{count}", "label": label,
                                "major_group": f"N={n}", "minor_group": f"{count}T",
                                "Thread speedup (×)": f"{float(row['thread_speedup']):.2f}"})
    if scaling:
        scaling.append({"group": "__caption__", "label": "", "major_group": "", "minor_group": "",
                        "Thread speedup (×)": "T = threads; each kernel and tile normalized to its one-thread runtime"})
        source = output / "thread_scaling.csv"
        write_csv(source, ["group", "label", "major_group", "minor_group", "Thread speedup (×)"], scaling)
        render(source, output / "thread_scaling", two_level=True)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--threads", type=int, default=4)
    args = parser.parse_args()
    plot_results(args.input, args.output, args.threads)
