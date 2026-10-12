#include "transpose.hpp"
#include "support.hpp"
#include <omp.h>

namespace transpose {
void parallel_naive(const Matrix& input, Matrix& output, Execution& execution) {
    const auto n = input.n;
    #pragma omp parallel
    {
        const int tid = omp_get_thread_num();
        #pragma omp single
        execution.threads = omp_get_num_threads();
        ThreadStats local;
        const double begin = lab::now();
        #pragma omp for schedule(static) nowait
        for (std::size_t i = 0; i < n; ++i) {
            for (std::size_t j = 0; j < n; ++j)
                output.values[j * n + i] = input.values[i * n + j];
            ++local.row_visits;
            local.updates += n;
        }
        const double finished = lab::now();
        #pragma omp barrier
        local.compute_s = finished - begin;
        local.wait_s = lab::now() - finished;
        execution.stats[tid] = local;
    }
}
}
