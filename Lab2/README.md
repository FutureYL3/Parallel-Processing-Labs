# Lab2：OpenMP 矩阵转置

将行优先的稠密方阵 A 转置到行优先方阵 B：

```text
A: N × N，uint32_t，行优先
B: N × N，uint32_t，行优先
B[j,i] = A[i,j]
```

各实现提供完整源码，程序直接执行矩阵生成、转置和计时。

## 源码结构

```text
Lab2/
  include/
    matrix.hpp                 方阵和输入生成接口
    transpose.hpp              转置内核和线程统计
    benchmark.hpp              运行参数
    support.hpp                计时、随机数和 CSV 工具
  src/
    main.cpp                   命令行参数
    matrix.cpp                 输入矩阵生成
    transpose_serial.cpp       串行转置
    transpose_parallel.cpp     按行并行转置
    transpose_tiled.cpp        分块并行转置
    benchmark.cpp              预热、测量和 CSV 输出
  run.py                       顺序测量和汇总
```

## 环境准备

进入 Lab2，后续命令都在这个文件夹内执行：

```bash
cd Lab2
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

默认扫描 N=2048、3072、4096、6144、8192 五组方阵。每组运行串行、按行并行，以及 tile=16、32、64、128 的分块转置，扫描 1、2、4 线程。编译产物保存在本实验的 `build/`，运行数据保存在本实验的 `results/`。

## 设置运行参数

以下参数用于 `run.py`：

| 参数 | 默认值 | 含义 |
|---|---|---|
| `--n` | 2048 3072 4096 6144 8192 | 方阵边长列表，可指定一个或多个值 |
| `--tile` | 16 32 64 128 | 转置分块边长列表，可指定一个或多个值 |
| `--threads` | 4 | 指定线程数，同时运行 1 线程；指定值不小于 2 时也运行 2 线程，重复值合并 |
| `--repeat` | 7 | 每次独立执行的测量次数 |
| `--rounds` | 3 | 测量批次数 |
| `--seed` | 2026 | 输入矩阵的随机种子 |
| `--output` | `results/` | 输出目录 |

指定全部主要参数：

```bash
python3 run.py --n 2048 3072 4096 6144 8192 --tile 16 32 64 128 \
  --threads 4 --repeat 7 --rounds 3 --seed 2026
```

修改 C++ 源码后，执行 `cmake --build build --parallel 4`，再运行 `python3 run.py`。

## 输入矩阵

输入和输出均为 N × N 的 `uint32_t` 方阵。输入由 `seed` 生成；相同 N 和 seed 下，各实现使用相同输入。矩阵元素按行存储，分块末尾的行列范围截断到 N。

## 单独运行一个实现

直接运行可执行文件，通过 `--kernel` 选择实现：

| 实现名称 | 参数 | 任务单位 |
|---|---|---|
| Serial | `--kernel serial` | 整个矩阵 |
| Naive | `--kernel naive` | 一行，OpenMP static 分配 |
| Tiled | `--kernel tiled` | tile × tile 区域，OpenMP static 分配 |

以下命令使用相同的矩阵参数，在 Bash 或 zsh 终端中执行：

```bash
transpose_args=(--n 2048 --tile 32 --threads 4 --repeat 7 --seed 2026)

./build/lab2_locality "${transpose_args[@]}" --kernel serial
./build/lab2_locality "${transpose_args[@]}" --kernel naive
./build/lab2_locality "${transpose_args[@]}" --kernel tiled
```

可执行文件向终端输出 CSV。保存到文件时，在命令末尾加上重定向：

```bash
mkdir -p results/single
./build/lab2_locality "${transpose_args[@]}" --kernel tiled > results/single/tiled.csv
```

## 分组运行

指定一组矩阵规模和多个分块边长：

```bash
python3 run.py --n 4096 --tile 16 32 64 128 --output results/n4096
```

指定多组矩阵规模和一个分块边长：

```bash
python3 run.py --n 2048 4096 8192 --tile 64 --output results/tile64
```

## 输出文件

`run.py` 默认将本次运行的数据保存到本实验的 `results/`，使用 `--output` 可以更改目录。

| 文件 | 内容 |
|---|---|
| `raw.csv` | 逐次耗时和每线程统计；`record` 区分 `summary`、`thread`、`trial` |
| `summary.csv` | 各矩阵规模、实现、分块边长和线程数的汇总数据 |
| `implementation_comparison.csv` | 各实现相对串行转置的加速比数据 |
| `thread_scaling.csv` | 同一实现相对自身单线程的加速比数据 |
| `environment.json` | 矩阵参数、编译器、CPU 和测量设置 |

默认每次独立执行先预热一次，再测量七次并取中位数；进行三个批次后再取中位数。计时包含并行区域启动、循环调度和屏障；矩阵生成、B 清零、输出汇总和 checksum 计算在计时之外。

常用 CSV 字段：

| 字段 | 含义 |
|---|---|
| `kernel`、`n`、`tile`、`threads` | 实现、矩阵边长、分块边长和实际线程数 |
| `variant` | 实现与分块配置的名称，例如 `Naive`、`Tile32` |
| `median_ms` | 汇总耗时，单位毫秒 |
| `speedup` | 串行转置耗时 / 当前方法耗时 |
| `thread_speedup` | 同实现、同 tile 的单线程耗时 / 多线程耗时 |
| `effective_gbps` | `2 × N × N × 4 / elapsed_s / 1e9`，按逻辑读写字节数计算 |
| `row_visits` | 行访问次数，分块实现包含不同列块对行的重复访问 |
| `tile_visits` | 处理的分块数 |
| `updates` | 写入的矩阵元素数 |
| `compute_s`、`wait_s` | 每线程计算和等待耗时，单位秒 |
| `checksum` | 输出矩阵的元素之和 |
