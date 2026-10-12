#pragma once
#include "spmm.hpp"

namespace spmm::detail {
// Each invocation owns C[begin:end, first:last]. C is zeroed by the caller.
inline void accumulate(const CsrMatrix& a, const DenseMatrix& b, DenseMatrix& c,
                       std::size_t begin, std::size_t end,
                       std::size_t first, std::size_t last, ThreadStats& stats) {
    stats.row_visits += end - begin;
    const auto nonzeros = a.row_ptr[end] - a.row_ptr[begin];
    stats.nnz_visits += nonzeros;
    stats.updates += static_cast<std::uint64_t>(nonzeros) * (last - first);
    for (std::size_t i = begin; i < end; ++i) {
        double* output = c.values.data() + i * c.cols;
        for (std::size_t p = a.row_ptr[i]; p < a.row_ptr[i + 1]; ++p) {
            const double value = a.values[p];
            const double* input = b.values.data() + static_cast<std::size_t>(a.col_idx[p]) * b.cols;
            // SIMD runs along the contiguous dense dimension, not CSR indices.
            #pragma omp simd
            for (std::size_t j = first; j < last; ++j)
                output[j] += value * input[j];
        }
    }
}
}
