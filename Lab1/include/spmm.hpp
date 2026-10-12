#pragma once
#include "matrix.hpp"

namespace spmm {
struct RowBlock {
    std::size_t begin, end, nnz;
};
struct ThreadStats {
    std::uint64_t row_visits = 0, nnz_visits = 0, updates = 0;
    double compute_s = 0.0, wait_s = 0.0;
};
struct Execution {
    int threads = 1;
    std::vector<ThreadStats> stats;
};

std::vector<RowBlock> partition_rows(const CsrMatrix& a, std::size_t target_nnz);
void serial(const CsrMatrix&, const DenseMatrix&, DenseMatrix&, Execution&);
void parallel_rows(const CsrMatrix&, const DenseMatrix&, DenseMatrix&, Execution&);
void parallel_balanced(const CsrMatrix&, const DenseMatrix&, DenseMatrix&,
                       const std::vector<RowBlock>&, Execution&);
void parallel_tiled(const CsrMatrix&, const DenseMatrix&, DenseMatrix&,
                    const std::vector<RowBlock>&, std::size_t tile, Execution&);

}
