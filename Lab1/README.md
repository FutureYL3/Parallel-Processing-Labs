# Lab1：OpenMP 稀疏矩阵乘法

计算 CSR 稀疏矩阵 A 与行优先稠密矩阵 B 的乘积：

```text
A: M × K，CSR
B: K × N，行优先
C: M × N，行优先
C = A × B
```

A 只存储非零元素。每个 `A[i,k]` 与 B 的第 k 行相乘，累加到 C 的第 i 行。各实现提供完整源码，程序直接执行矩阵生成、乘法和计时。

## 源码结构

```text
Lab1/
  include/
    matrix.hpp                 CSR 和稠密矩阵
    spmm.hpp                   内核、行块和线程统计
    benchmark.hpp              运行参数
    support.hpp                计时、随机数和 CSV 工具
    detail/row_kernel.hpp      CSR 乘加与 SIMD 列循环
  src/
    main.cpp                   命令行参数
    matrix.cpp                 输入矩阵生成
    partition.cpp              按非零数构造行块
    spmm_serial.cpp            串行 CSR 内核
    spmm_parallel.cpp          按行并行与行块并行
    spmm_tiled.cpp             行块、列块与 SIMD 微块
    benchmark.cpp              预热、测量和 CSV 输出
  run.py                       顺序测量和汇总
```

## 环境准备

进入 Lab1，后续命令都在这个文件夹内执行：

```bash
cd Lab1
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

默认 M=4096、K=8192、N=1024、平均每行 32 个非零元素，运行三种输入分布和各个实现，扫描 1、2、4 线程。编译产物保存在本实验的 `build/`，运行数据保存在本实验的 `results/`。

## 设置运行参数

以下参数用于 `run.py`：

| 参数 | 默认值 | 含义 |
|---|---|---|
| `--m` | 4096 | A 和 C 的行数 |
| `--k` | 8192 | A 的列数、B 的行数 |
| `--n` | 1024 | B 和 C 的列数 |
| `--nnz-per-row` | 32 | A 平均每行的非零元素数 |
| `--threads` | 4 | 指定线程数，同时运行 1 线程；指定值不小于 2 时也运行 2 线程，重复值合并 |
| `--block-nnz` | 256 | 构造行块时的目标非零元素数 |
| `--tile` | 128 | 输出列块宽度 |
| `--repeat` | 7 | 每次独立执行的测量次数 |
| `--rounds` | 3 | 测量批次数 |
| `--seed` | 2026 | 输入矩阵的随机种子 |
| `--output` | `results/` | 输出目录 |

指定全部主要参数：

```bash
python3 run.py --m 4096 --k 8192 --n 1024 \
  --nnz-per-row 32 --threads 4 --tile 128 --block-nnz 256 \
  --repeat 7 --rounds 3 --seed 2026
```

修改 C++ 源码后，执行 `cmake --build build --parallel 4`，再运行 `python3 run.py`。

## 输入分布

- `uniform`：各行非零数相同，列位置分散。
- `skewed`：较多非零元素集中在前约 1/8 行。
- `blocked`：每 32 行共享紧凑列窗口。

相同维度和参数下，三个 profile 都保存 `M × nnz-per-row` 个非零元素。每种方法使用相同 seed、A、B。`blocked` 是输入分布名称，存储格式始终为 CSR。

## 单独运行一个实现

直接运行可执行文件，通过 `--kernel`、`--schedule` 和 `--profile` 选择实现、调度方式和输入分布。

| 实现名称 | 参数 | 任务单位 |
|---|---|---|
| Serial | `--kernel serial` | 整个矩阵 |
| Rows | `--kernel rows --schedule static` | 一行 |
| Dynamic | `--kernel rows --schedule dynamic --chunk 1` | 一行 |
| NNZ blocks | `--kernel balanced --schedule static` | 连续行块 |
| Tiled | `--kernel tiled --schedule static` | 行块 × 输出列块 |

以下命令使用相同的矩阵参数，在 Bash 或 zsh 终端中执行：

```bash
spmm_args=(--m 4096 --k 8192 --n 1024 --nnz-per-row 32
  --profile skewed --threads 4 --repeat 7 --seed 2026
  --block-nnz 256 --tile 128)

./build/lab1_spmm "${spmm_args[@]}" --kernel serial
./build/lab1_spmm "${spmm_args[@]}" --kernel rows --schedule static
./build/lab1_spmm "${spmm_args[@]}" --kernel rows --schedule dynamic --chunk 1
./build/lab1_spmm "${spmm_args[@]}" --kernel balanced --schedule static
./build/lab1_spmm "${spmm_args[@]}" --kernel tiled --schedule static
```

将 `--profile skewed` 替换为 `uniform` 或 `blocked`，即可切换输入分布。

可执行文件向终端输出 CSV。保存到文件时，在命令末尾加上重定向：

```bash
mkdir -p results/single
./build/lab1_spmm "${spmm_args[@]}" --kernel tiled --schedule static \
  > results/single/tiled.csv
```

## 分组运行

改变行块大小，将每组输出保存到独立目录：

```bash
for block in 64 256 1024; do
  python3 run.py --block-nnz "$block" --output "results/block_$block"
done
```

改变列块宽度：

```bash
for tile in 32 64 128 256; do
  python3 run.py --tile "$tile" --output "results/tile_$tile"
done
```

## 输出文件

`run.py` 默认将本次运行的数据保存到本实验的 `results/`，使用 `--output` 可以更改目录。

| 文件 | 内容 |
|---|---|
| `raw.csv` | 逐次耗时和每线程统计；`record` 区分 `summary`、`thread`、`trial` |
| `summary.csv` | 各输入、实现和线程数的汇总数据 |
| `implementation_comparison.csv` | 各实现相对串行 CSR 的加速比数据 |
| `thread_scaling.csv` | 同一实现相对自身单线程的加速比数据 |
| `environment.json` | 矩阵参数、编译器、CPU 和测量设置 |

默认每次独立执行先预热一次，再测量七次并取中位数；进行三个批次后再取中位数。计时包含并行区域启动、循环调度和屏障；矩阵生成、C 清零、行块构造和输出汇总在计时之外。

常用 CSV 字段：

| 字段 | 含义 |
|---|---|
| `median_ms` | 汇总耗时，单位毫秒 |
| `speedup` | 对应输入的串行 CSR 耗时 / 当前方法耗时 |
| `thread_speedup` | 当前方法单线程耗时 / 当前方法多线程耗时 |
| `gflops` | `2 × nnz × N / elapsed_s / 1e9` |
| `partition_s` | 行块构造耗时，单位秒 |
| `updates` | 标量乘加次数 |
| `nnz_visits` | 非零元素访问次数 |
| `compute_s`、`wait_s` | 每线程计算和等待耗时，单位秒 |
| `checksum` | 输出矩阵的元素之和 |
