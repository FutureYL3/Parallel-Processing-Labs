#include "detail/row_kernel.hpp"
#include "support.hpp"
#include <omp.h>

namespace spmm {
void parallel_rows(const CsrMatrix& a, const DenseMatrix& b, DenseMatrix& c, Execution& execution) {
    #pragma omp parallel
    {
        const int tid = omp_get_thread_num();
        #pragma omp single
        execution.threads = omp_get_num_threads();
        ThreadStats local;
        const double begin = lab::now();
        #pragma omp for schedule(runtime) nowait
        for (std::size_t i = 0; i < a.rows; ++i)
            detail::accumulate(a, b, c, i, i + 1, 0, b.cols, local);
        const double finished = lab::now();
        #pragma omp barrier
        local.compute_s = finished - begin; local.wait_s = lab::now() - finished;
        execution.stats[tid] = local;
    }
}

void parallel_balanced(const CsrMatrix& a, const DenseMatrix& b, DenseMatrix& c,
                       const std::vector<RowBlock>& blocks, Execution& execution) {
    #pragma omp parallel
    {
        const int tid = omp_get_thread_num();
        #pragma omp single
        execution.threads = omp_get_num_threads();
        ThreadStats local;
        const double begin = lab::now();
        #pragma omp for schedule(runtime) nowait
        for (std::size_t task = 0; task < blocks.size(); ++task)
            detail::accumulate(a, b, c, blocks[task].begin, blocks[task].end, 0, b.cols, local);
        const double finished = lab::now();
        #pragma omp barrier
        local.compute_s = finished - begin; local.wait_s = lab::now() - finished;
        execution.stats[tid] = local;
    }
}
}
