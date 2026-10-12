# Lab3：OpenMP 直方图

对整数输入统计直方图：

```text
input: N 个整数，每个值属于 [0, bins)
counts: bins 个 uint64_t 计数
counts[input[i]] += 1
```

各实现提供完整源码，程序直接执行输入生成、计数和计时。

## 源码结构

```text
Lab3/
  include/
    input.hpp                  输入和计数类型
    histogram.hpp              计数内核、工作空间和线程统计
    benchmark.hpp              运行参数
    support.hpp                计时、随机数和 CSV 工具
  src/
    main.cpp                   命令行参数
    input.cpp                  输入生成
    workspace.cpp              分段锁和线程私有计数空间
    histogram_serial.cpp       串行计数
    histogram_critical.cpp     全局临界区
    histogram_atomic.cpp       原子加
    histogram_locks.cpp        分段锁
    histogram_private.cpp      线程私有计数和合并
    benchmark.cpp              预热、测量和 CSV 输出
  run.py                       顺序测量和汇总
```

## 环境准备

进入 Lab3，后续命令都在这个文件夹内执行：

```bash
cd Lab3
```

Windows 使用 WSL2 的 Ubuntu 终端安装依赖：

```bash
sudo apt update
sudo apt install -y g++ cmake python3 python3-venv
```

macOS 安装依赖并选择支持 OpenMP 的编译器：

```bash
brew install cmake python llvm libomp
export CXX="$(brew --prefix llvm)/bin/clang++"
export OpenMP_ROOT="$(brew --prefix libomp)"
```

安装本实验的 Python 依赖：

```bash
python3 -m venv .venv
source .venv/bin/activate
python3 -m pip install -r requirements.txt
```

## 构建与运行

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 4
python3 run.py
```

默认扫描 100 万、500 万、1000 万个输入，运行 uniform 和 hotspot 两种分布。每组输入运行串行、全局临界区、原子加、1/4/16/64 把分段锁和线程私有计数，扫描 1、2、4 线程。编译产物保存在本实验的 `build/`，运行数据保存在本实验的 `results/`。

## 设置运行参数

以下参数用于 `run.py`：

| 参数 | 默认值 | 含义 |
|---|---|---|
| `--n` | 1000000 5000000 10000000 | 输入元素数列表 |
| `--bins` | 256 | bucket 数 |
| `--stripes` | 1 4 16 64 | 分段锁数量列表 |
| `--distribution` | uniform hotspot | 输入分布列表 |
| `--hot-percent` | 90 | hotspot 分布中直接映射到 bucket 0 的比例，单位百分比 |
| `--threads` | 4 | 指定线程数，同时运行 1 线程；指定值不小于 2 时也运行 2 线程，重复值合并 |
| `--repeat` | 7 | 每次独立执行的测量次数 |
| `--rounds` | 3 | 测量批次数 |
| `--seed` | 2026 | 输入随机种子 |
| `--output` | `results/` | 输出目录 |

指定全部主要参数：

```bash
python3 run.py --n 1000000 5000000 10000000 --bins 256 --stripes 1 4 16 64 \
  --distribution uniform hotspot --hot-percent 90 --threads 4 \
  --repeat 7 --rounds 3 --seed 2026
```

修改 C++ 源码后，执行 `cmake --build build --parallel 4`，再运行 `python3 run.py`。

## 输入分布

- `uniform`：输入分散到全部 bucket。
- `hotspot`：按 `hot-percent` 指定的比例映射到 bucket 0，其余输入分散到全部 bucket。

hotspot 剩余输入也可能映射到 bucket 0，因此实际 bucket 0 占比略高于设置值。相同 N、bins、distribution、hot-percent 和 seed 下，各实现使用相同输入。

## 单独运行一个实现

直接运行可执行文件，通过 `--kernel`、`--distribution` 和 `--stripes` 选择实现、分布和锁数：

| 实现名称 | 参数 | 更新方式 |
|---|---|---|
| Serial | `--kernel serial` | 单线程计数 |
| Critical | `--kernel critical` | 每次更新进入同一个全局临界区 |
| Atomic | `--kernel atomic` | 每个计数使用原子加 |
| Lock16 | `--kernel locks --stripes 16` | bucket 按 `bucket % stripes` 映射到分段锁 |
| Private | `--kernel private` | 每线程独立计数，最后按 bucket 合并 |

以下命令使用相同的输入参数，在 Bash 或 zsh 终端中执行：

```bash
histogram_args=(--n 10000000 --bins 256 --distribution uniform
  --hot-percent 90 --threads 4 --repeat 7 --seed 2026)

./build/lab3_histogram "${histogram_args[@]}" --kernel serial
./build/lab3_histogram "${histogram_args[@]}" --kernel critical
./build/lab3_histogram "${histogram_args[@]}" --kernel atomic
./build/lab3_histogram "${histogram_args[@]}" --kernel locks --stripes 16
./build/lab3_histogram "${histogram_args[@]}" --kernel private
```

将 `--distribution uniform` 替换为 `hotspot`，即可切换输入分布。将 `--stripes 16` 替换为其他锁数，运行对应的分段锁配置。

可执行文件向终端输出 CSV。保存到文件时，在命令末尾加上重定向：

```bash
mkdir -p results/single
./build/lab3_histogram "${histogram_args[@]}" --kernel private > results/single/private.csv
```

## 分组运行

固定输入规模，指定多组锁数：

```bash
python3 run.py --n 10000000 --stripes 1 4 16 64 256 --output results/lock_counts
```

固定锁数，指定多组输入规模：

```bash
python3 run.py --n 1000000 5000000 10000000 --stripes 16 --output results/lock16
```

改变热点比例：

```bash
for hot in 50 90 99; do
  python3 run.py --n 10000000 --distribution hotspot --hot-percent "$hot" \
    --output "results/hot_$hot"
done
```

## 输出文件

`run.py` 默认将本次运行的数据保存到本实验的 `results/`，使用 `--output` 可以更改目录。

| 文件 | 内容 |
|---|---|
| `raw.csv` | 逐次耗时和每线程统计；`record` 区分 `summary`、`thread`、`trial` |
| `summary.csv` | 各输入规模、分布、实现、锁数和线程数的汇总数据 |
| `implementation_comparison.csv` | 各实现相对匹配输入的串行耗时的加速比数据 |
| `thread_scaling.csv` | 同实现、同锁数相对自身单线程的加速比数据 |
| `lock_count_comparison.csv` | 固定线程数下，各锁数相对本组最小锁数的加速比数据 |
| `environment.json` | 输入参数、编译器、CPU 和测量设置 |

默认每次独立执行先预热一次，再测量七次并取中位数；进行三个批次后再取中位数。计时包含并行区域启动、循环调度、计数更新、线程屏障和私有计数合并；输入生成、表清零、锁初始化、输出汇总和 checksum 计算在计时之外。

常用 CSV 字段：

| 字段 | 含义 |
|---|---|
| `kernel`、`distribution`、`n`、`bins`、`stripes`、`threads` | 实现、分布、规模、bucket 数、锁数和实际线程数 |
| `variant` | 实现与锁数配置的名称，例如 `Critical`、`Lock16` |
| `median_ms` | 汇总耗时，单位毫秒 |
| `speedup` | 匹配输入的串行耗时 / 当前方法耗时 |
| `thread_speedup` | 同实现、同锁数的单线程耗时 / 多线程耗时 |
| `lock_count_speedup` | 同线程数下，本组最小锁数的耗时 / 当前锁数的耗时 |
| `lock_count_baseline` | 锁数对比所使用的基线锁数 |
| `million_items_s` | 每秒处理的百万输入元素数 |
| `updates` | 处理的输入元素数 |
| `merged_bins` | 合并的 bucket 数 |
| `compute_s` | 计数和合并阶段耗时，包含计数阶段的锁等待 |
| `wait_s` | 线程屏障等待耗时 |
| `merge_s` | 私有计数合并阶段耗时，已包含在 `compute_s` 中 |
| `checksum` | 全部 bucket 的计数之和 |
