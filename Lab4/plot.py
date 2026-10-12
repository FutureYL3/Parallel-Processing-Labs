#!/usr/bin/env python3
"""Export distributed SpMM implementation, process-count and communication comparisons."""
import argparse
import csv
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE / "third_party/golden_ratio"))
from golden_ratio_plot.config import PHI, PlotConfig
from golden_ratio_plot.reader import AblationData, read_csv
from golden_ratio_plot.renderer.ablation import AblationRenderer
from golden_ratio_plot.utils.ticks import nice_range

VARIANTS = ["Broadcast", "Allgather", "Halo", "Overlap"]
PROFILES = {"uniform": "Uniform", "skewed": "Skewed", "blocked": "Blocked", "banded": "Banded"}


def write_csv(path, fields, rows):
    with path.open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=fields)
        writer.writeheader(); writer.writerows(rows)


def render(source, output):
    config = PlotConfig(input=str(source), output=str(output), mode="ablation",
                        width_pt=540, font_size_pt=7, y_min=0, show_values=False,
                        two_level_xaxis=True, formats=["pdf", "png", "svg"])
    AblationRenderer(config).render(read_csv(source))


def render_process_scaling(source, output, sizes):
    data = read_csv(source)
    panels = []
    for m in sizes:
        groups = [group for group in data.groups if group.startswith(f"{m}-")]
        panels.append(AblationData(
            groups=groups, labels=data.labels,
            data={key: value for key, value in data.data.items() if key[0] in groups},
            value_label=data.value_label,
            major_group={group: PROFILES[group.split("-")[1]] for group in groups},
            minor_group={group: data.minor_group[group] for group in groups},
            caption=f"M={m}; P = processes; relative to each method's one-process runtime"))
    _, upper, _ = nice_range(0, max(data.all_values()))
    config = PlotConfig(input=str(source), output=str(output), mode="ablation",
                        width_pt=540, height_pt=240 / PHI * len(panels), font_size_pt=7,
                        y_min=0, y_max=upper,
                        show_values=False, two_level_xaxis=True,
                        formats=["pdf", "png", "svg"])
    AblationRenderer(config).render_panels(panels)


def plot_results(summary_path, output, ranks):
    output = Path(output); output.mkdir(parents=True, exist_ok=True)
    with Path(summary_path).open() as stream: rows = list(csv.DictReader(stream))
    sizes = sorted({int(row["m"]) for row in rows})
    profiles = [profile for profile in PROFILES if any(row["profile"] == profile for row in rows)]
    counts = sorted({int(row["ranks"]) for row in rows if int(row["ranks"]) > 1})

    def find(m, profile, label, count):
        return next(row for row in rows if int(row["m"]) == m and row["profile"] == profile
                    and row["variant"] == label and int(row["ranks"]) == count)

    comparison, scaling, communication = [], [], []
    for m in sizes:
        for profile in profiles:
            major, minor = f"M={m}", PROFILES[profile]
            group = f"{m}-{profile}"
            comparison.append({"group": group, "label": "Serial", "major_group": major,
                               "minor_group": minor, "Speedup over serial (×)": "1"})
            for label in VARIANTS:
                row = find(m, profile, label, ranks)
                comparison.append({"group": group, "label": label, "major_group": major,
                                   "minor_group": minor, "Speedup over serial (×)": f"{float(row['speedup']):.6g}"})
                communication.append({"group": group, "label": label, "major_group": major,
                                      "minor_group": minor, "Received payload (MiB)": f"{float(row['received_mib']):.6g}"})
            for count in counts:
                for label in VARIANTS:
                    row = find(m, profile, label, count)
                    scaling.append({"group": f"{group}-{count}", "label": label,
                                    "major_group": f"{major} {minor}", "minor_group": f"{count}P",
                                    "Process speedup (×)": f"{float(row['process_speedup']):.6g}"})
    comparison.append({"group": "__caption__", "label": "", "major_group": "", "minor_group": "",
                       "Speedup over serial (×)": f"{ranks}-process SpMM; B exchange and packing included; same local tiled kernel"})
    source = output / "implementation_comparison.csv"
    write_csv(source, ["group", "label", "major_group", "minor_group", "Speedup over serial (×)"], comparison)
    render(source, output / "implementation_comparison")
    if scaling:
        scaling.append({"group": "__caption__", "label": "", "major_group": "", "minor_group": "",
                        "Process speedup (×)": "P = processes; each method normalized to its own one-process runtime"})
        source = output / "process_scaling.csv"
        write_csv(source, ["group", "label", "major_group", "minor_group", "Process speedup (×)"], scaling)
        render_process_scaling(source, output / "process_scaling", sizes)
    communication.append({"group": "__caption__", "label": "", "major_group": "", "minor_group": "",
                          "Received payload (MiB)": f"{ranks} processes; sum of received B payload across ranks per multiply; metadata excluded"})
    source = output / "communication_volume.csv"
    write_csv(source, ["group", "label", "major_group", "minor_group", "Received payload (MiB)"], communication)
    render(source, output / "communication_volume")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--ranks", type=int, default=4)
    args = parser.parse_args()
    plot_results(args.input, args.output, args.ranks)
