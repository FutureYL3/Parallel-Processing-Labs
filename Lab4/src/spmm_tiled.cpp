#include "spmm.hpp"
#include <algorithm>
#include <array>

namespace spmm {
namespace {
template<std::size_t Width>
void micro_tile(const CsrMatrix& a, const DenseMatrix& b, DenseMatrix& c,
                 std::size_t row, std::size_t column, bool add) {
    std::array<double, Width> sums{};
    for (std::size_t p = a.row_ptr[row]; p < a.row_ptr[row + 1]; ++p) {
        const double value = a.values[p];
        const auto* input = b.values.data() + static_cast<std::size_t>(a.col_idx[p]) * b.cols + column;
        for (std::size_t lane = 0; lane < Width; ++lane) sums[lane] += value * input[lane];
    }
    auto* output = c.values.data() + row * c.cols + column;
    for (std::size_t lane = 0; lane < Width; ++lane)
        output[lane] = sums[lane] + (add ? output[lane] : 0.0);
}
}

void multiply_tiled(const CsrMatrix& a, const DenseMatrix& b, DenseMatrix& c,
                     const std::vector<RowBlock>& blocks, std::size_t tile, bool add) {
    for (const auto& block : blocks)
        for (std::size_t first = 0; first < b.cols; first += tile) {
            const auto last = std::min(first + tile, b.cols);
            for (std::size_t row = block.begin; row < block.end; ++row) {
                std::size_t column = first;
                for (; column + 32 <= last; column += 32) micro_tile<32>(a, b, c, row, column, add);
                for (; column + 8 <= last; column += 8) micro_tile<8>(a, b, c, row, column, add);
                switch (last - column) {
                    case 7: micro_tile<7>(a, b, c, row, column, add); break;
                    case 6: micro_tile<6>(a, b, c, row, column, add); break;
                    case 5: micro_tile<5>(a, b, c, row, column, add); break;
                    case 4: micro_tile<4>(a, b, c, row, column, add); break;
                    case 3: micro_tile<3>(a, b, c, row, column, add); break;
                    case 2: micro_tile<2>(a, b, c, row, column, add); break;
                    case 1: micro_tile<1>(a, b, c, row, column, add); break;
                    default: break;
                }
            }
        }
}
}
