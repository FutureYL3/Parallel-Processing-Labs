#include "histogram.hpp"
#include "support.hpp"

namespace histogram {
void critical(const Input& input, Counts& counts, Execution& execution) {
    #pragma omp parallel
    {
        const int tid = omp_get_thread_num();
        #pragma omp single
        execution.threads = omp_get_num_threads();
        ThreadStats local;
        const double begin = lab::now();
        #pragma omp for schedule(static) nowait
        for (std::size_t i = 0; i < input.size(); ++i) {
            const auto bucket = input[i];
            #pragma omp critical(histogram_update)
            { ++counts[bucket]; }
            ++local.updates;
        }
        const double finished = lab::now();
        #pragma omp barrier
        local.compute_s = finished - begin;
        local.wait_s = lab::now() - finished;
        execution.stats[tid] = local;
    }
}
}
