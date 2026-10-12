#include "histogram.hpp"
#include "support.hpp"

namespace histogram {
void private_counts(const Input& input, Counts& counts, Workspace& workspace, Execution& execution) {
    #pragma omp parallel
    {
        const int tid = omp_get_thread_num();
        #pragma omp single
        execution.threads = omp_get_num_threads();
        auto* local_counts = workspace.local_counts.data() + static_cast<std::size_t>(tid) * workspace.stride;
        ThreadStats local;
        const double begin = lab::now();
        #pragma omp for schedule(static) nowait
        for (std::size_t i = 0; i < input.size(); ++i) {
            ++local_counts[input[i]];
            ++local.updates;
        }
        const double counted = lab::now();
        #pragma omp barrier
        const double merge_begin = lab::now();
        #pragma omp for schedule(static) nowait
        for (std::size_t bucket = 0; bucket < counts.size(); ++bucket) {
            std::uint64_t total = 0;
            for (int t = 0; t < execution.threads; ++t)
                total += workspace.local_counts[static_cast<std::size_t>(t) * workspace.stride + bucket];
            counts[bucket] = total;
            ++local.merged_bins;
        }
        const double merged = lab::now();
        #pragma omp barrier
        local.merge_s = merged - merge_begin;
        local.compute_s = counted - begin + local.merge_s;
        local.wait_s = merge_begin - counted + lab::now() - merged;
        execution.stats[tid] = local;
    }
}
}
