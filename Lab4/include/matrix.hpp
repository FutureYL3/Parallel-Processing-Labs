#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace spmm {
struct CsrMatrix {
    std::size_t rows = 0, cols = 0;
    std::vector<std::size_t> row_ptr;
    std::vector<std::uint32_t> col_idx;
    std::vector<double> values;
};
struct DenseMatrix {
    std::size_t rows = 0, cols = 0;
    std::vector<double> values;
    DenseMatrix(std::size_t m = 0, std::size_t n = 0);
};
struct RowBlock { std::size_t begin, end; };

std::size_t row_nonzeros(std::size_t row, std::size_t m, std::size_t k,
                         std::size_t nnz_per_row, const std::string& profile);
CsrMatrix generate_sparse_rows(std::size_t begin, std::size_t end, std::size_t m,
                               std::size_t k, std::size_t nnz_per_row,
                               const std::string& profile, std::uint32_t seed);
DenseMatrix generate_dense_rows(std::size_t begin, std::size_t end,
                                std::size_t n, std::uint32_t seed);
std::vector<RowBlock> make_row_blocks(const CsrMatrix&, std::size_t target_nnz);
}
