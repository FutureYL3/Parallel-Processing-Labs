#include "spmm.hpp"

namespace spmm {
std::vector<RowBlock> partition_rows(const CsrMatrix& a, std::size_t target_nnz) {
    std::vector<RowBlock> blocks;
    std::size_t begin = 0;
    for (std::size_t i = 0; i < a.rows; ++i) {
        const auto count = a.row_ptr[i + 1] - a.row_ptr[begin];
        if (count >= target_nnz) { blocks.push_back({begin, i + 1, count}); begin = i + 1; }
    }
    if (begin < a.rows) blocks.push_back({begin, a.rows, a.row_ptr.back() - a.row_ptr[begin]});
    return blocks;
}
}
