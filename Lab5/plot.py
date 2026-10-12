#!/usr/bin/env python3
"""Export tile/buffer, array-size and DRAM-traffic comparisons."""
import argparse
import csv
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent
sys.path.insert(0,str(HERE/"third_party/golden_ratio"))
from golden_ratio_plot.config import PlotConfig
from golden_ratio_plot.reader import read_csv
from golden_ratio_plot.renderer.ablation import AblationRenderer


def write_csv(path, fields, rows):
    with path.open("w",newline="",encoding="utf-8") as stream:
        writer=csv.DictWriter(stream,fieldnames=fields)
        writer.writeheader();writer.writerows(rows)


def render(source, output):
    config=PlotConfig(input=str(source),output=str(output),mode="ablation",
                      width_pt=540,font_size_pt=7,y_min=0,show_values=False,
                      two_level_xaxis=True,formats=["pdf","png","svg"])
    AblationRenderer(config).render(read_csv(source))


def tile_label(row):
    tm,tk,tn=[int(row[key]) for key in ["tile_m","tile_k","tile_n"]]
    return f"T{tm}" if tm==tk==tn else f"{tm}x{tk}x{tn}"


def plot_results(source, output):
    output=Path(output)
    output.mkdir(parents=True,exist_ok=True)
    with Path(source).open() as stream: rows=list(csv.DictReader(stream))
    shapes=sorted({(int(r['m']),int(r['k']),int(r['n'])) for r in rows})
    arrays=sorted({int(r['array']) for r in rows})
    comparison,traffic,scaling,overlap=[],[],[],[]
    for shape in shapes:
        m,k,n=shape
        group_shape=f"{m}x{k}x{n}"
        selected=[r for r in rows if (int(r['m']),int(r['k']),int(r['n']))==shape]
        selected.sort(key=lambda r: (int(r['tile_m']),int(r['tile_k']),int(r['tile_n']),
                                     r['buffering']!=r['baseline_buffering']))
        for array in arrays:
            for row in selected:
                if int(row['array'])!=array: continue
                group=f"{group_shape}-{array}"
                label=f"{row['buffering'].capitalize()} {tile_label(row)}"
                fields={'group':group,'label':label,'major_group':group_shape,'minor_group':f"{array}x{array}"}
                comparison.append({**fields,'Cycle speedup (×)':f"{float(row['speedup']):.6g}"})
                traffic.append({**fields,'DRAM traffic (MiB)':f"{float(row['read_mib'])+float(row['write_mib']):.6g}"})
                if row['buffering']=='double':
                    base=next((r for r in selected if int(r['array'])==array and r['buffering']=='single'
                               and all(r[key]==row[key] for key in ['tile_m','tile_k','tile_n'])),None)
                    if base:
                        overlap.append({'group':group,'label':tile_label(row),'major_group':group_shape,
                                        'minor_group':f"{array}x{array}",
                                        'Buffer speedup (×)':f"{int(base['cycles'])/int(row['cycles']):.6g}"})
        buffering='double' if any(r['buffering']=='double' for r in selected) else selected[0]['buffering']
        configs=sorted({tuple(int(r[key]) for key in ['tile_m','tile_k','tile_n']) for r in selected})
        for config in configs:
            applicable=[r for r in selected if r['buffering']==buffering
                        and tuple(int(r[key]) for key in ['tile_m','tile_k','tile_n'])==config]
            baseline=min(applicable,key=lambda r:int(r['array']))
            for row in sorted(applicable,key=lambda r:int(r['array'])):
                scaling.append({'group':f"{group_shape}-{config}",'label':f"{row['array']}x{row['array']}",
                                'major_group':group_shape,'minor_group':tile_label(row),
                                'Array speedup (×)':f"{int(baseline['cycles'])/int(row['cycles']):.6g}"})
    base=rows[0]
    baseline_tile=f"{base['baseline_tile_m']}x{base['baseline_tile_k']}x{base['baseline_tile_n']}"
    caption=f"Each array normalized to {base['baseline_buffering']} buffering, tile {baseline_tile}; DRAM included"
    for name,data,value_label,caption_text in [
        ('implementation_comparison',comparison,'Cycle speedup (×)',caption),
        ('array_scaling',scaling,'Array speedup (×)',f"Each tile normalized to {arrays[0]}x{arrays[0]}; double buffering when available"),
        ('dram_traffic',traffic,'DRAM traffic (MiB)','Read plus written payload per GEMM; 64-bit read beats include padding'),
        ('buffering_comparison',overlap,'Buffer speedup (×)','Single-buffer cycles / double-buffer cycles; matched array, matrix and tile')]:
        if not data: continue
        data.append({'group':'__caption__','label':'','major_group':'','minor_group':'',value_label:caption_text})
        csv_path=output/(name+'.csv')
        write_csv(csv_path,['group','label','major_group','minor_group',value_label],data)
        render(csv_path,output/name)


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();plot_results(args.input,args.output)
