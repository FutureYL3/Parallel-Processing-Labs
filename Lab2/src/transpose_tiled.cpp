#include "transpose.hpp"
#include "support.hpp"
#include <omp.h>

namespace transpose {
void parallel_tiled(const Matrix& input, Matrix& output, std::size_t tile, Execution& execution) {
    const auto n = input.n;
    #pragma omp parallel
    {
        const int tid = omp_get_thread_num();
        #pragma omp single
        execution.threads = omp_get_num_threads();
        ThreadStats local;
        const double begin = lab::now();
        #pragma omp for collapse(2) schedule(static) nowait
        for (std::size_t bi = 0; bi < n; bi += tile)
            for (std::size_t bj = 0; bj < n; bj += tile) {
                const auto row_end = std::min(bi + tile, n);
                const auto column_end = std::min(bj + tile, n);
                for (std::size_t i = bi; i < row_end; ++i)
                    for (std::size_t j = bj; j < column_end; ++j)
                        output.values[j * n + i] = input.values[i * n + j];
                ++local.tile_visits;
                local.row_visits += row_end - bi;
                local.updates += (row_end - bi) * (column_end - bj);
            }
        const double finished = lab::now();
        #pragma omp barrier
        local.compute_s = finished - begin;
        local.wait_s = lab::now() - finished;
        execution.stats[tid] = local;
    }
}
}
