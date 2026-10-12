#include "detail/row_kernel.hpp"
#include "support.hpp"
#include <array>
#include <omp.h>

namespace spmm {
namespace {
constexpr std::size_t micro_width = 32;

// Keep 32 output values in local SIMD accumulators across the CSR row.
// Unlike repeated C += A*B, each output element is stored only once.
template<std::size_t Width>
void micro_tile(const CsrMatrix& a, const DenseMatrix& b, DenseMatrix& c,
                std::size_t row, std::size_t column) {
    std::array<double, Width> sums{};
    for (std::size_t p = a.row_ptr[row]; p < a.row_ptr[row + 1]; ++p) {
        const double value = a.values[p];
        const double* input = b.values.data() + static_cast<std::size_t>(a.col_idx[p]) * b.cols + column;
        #pragma omp simd
        for (std::size_t lane = 0; lane < Width; ++lane) sums[lane] += value * input[lane];
    }
    double* output = c.values.data() + row * c.cols + column;
    #pragma omp simd
    for (std::size_t lane = 0; lane < Width; ++lane) output[lane] = sums[lane];
}

void register_block(const CsrMatrix& a, const DenseMatrix& b, DenseMatrix& c,
                    std::size_t begin, std::size_t end, std::size_t first, std::size_t last,
                    ThreadStats& stats) {
    const auto nonzeros = a.row_ptr[end] - a.row_ptr[begin];
    stats.row_visits += end - begin;
    const auto width = last - first;
    const auto tail = width % micro_width;
    stats.nnz_visits += nonzeros * (width / micro_width + tail / 8 + (tail % 8 != 0));
    stats.updates += static_cast<std::uint64_t>(nonzeros) * (last - first);
    for (std::size_t row = begin; row < end; ++row) {
        std::size_t column = first;
        for (; column + micro_width <= last; column += micro_width)
            micro_tile<micro_width>(a, b, c, row, column);
        for (; column + 8 <= last; column += 8)
            micro_tile<8>(a, b, c, row, column);
        // Tiny tail kernels keep partial panels in bounds without padding.
        switch (last - column) {
            case 7: micro_tile<7>(a, b, c, row, column); break;
            case 6: micro_tile<6>(a, b, c, row, column); break;
            case 5: micro_tile<5>(a, b, c, row, column); break;
            case 4: micro_tile<4>(a, b, c, row, column); break;
            case 3: micro_tile<3>(a, b, c, row, column); break;
            case 2: micro_tile<2>(a, b, c, row, column); break;
            case 1: micro_tile<1>(a, b, c, row, column); break;
            default: break;
        }
    }
}
}

void parallel_tiled(const CsrMatrix& a, const DenseMatrix& b, DenseMatrix& c,
                    const std::vector<RowBlock>& blocks, std::size_t tile, Execution& execution) {
    const auto column_tiles = (b.cols + tile - 1) / tile;
    #pragma omp parallel
    {
        const int tid = omp_get_thread_num();
        #pragma omp single
        execution.threads = omp_get_num_threads();
        ThreadStats local;
        const double begin = lab::now();
        // Each task owns a disjoint rectangle of C; no atomic updates needed.
        #pragma omp for collapse(2) schedule(runtime) nowait
        for (std::size_t block = 0; block < blocks.size(); ++block)
            for (std::size_t panel = 0; panel < column_tiles; ++panel) {
                const auto first = panel * tile;
                const auto last = std::min(first + tile, b.cols);
                register_block(a, b, c, blocks[block].begin, blocks[block].end, first, last, local);
            }
        const double finished = lab::now();
        #pragma omp barrier
        local.compute_s = finished - begin; local.wait_s = lab::now() - finished;
        execution.stats[tid] = local;
    }
}
}
