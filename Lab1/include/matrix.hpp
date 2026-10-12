#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace spmm {
// A is M x K. Nonzeros in each row are sorted by column, with no duplicates.
struct CsrMatrix {
    std::size_t rows = 0, cols = 0;
    std::vector<std::size_t> row_ptr;
    std::vector<std::uint32_t> col_idx;
    std::vector<double> values;
};

struct DenseMatrix {
    std::size_t rows = 0, cols = 0;
    std::vector<double> values;
    DenseMatrix(std::size_t m, std::size_t n);
};

CsrMatrix generate_sparse(std::size_t m, std::size_t k, std::size_t nnz_per_row,
                          const std::string& profile, std::uint32_t seed);
DenseMatrix generate_dense(std::size_t k, std::size_t n, std::uint32_t seed);
}
