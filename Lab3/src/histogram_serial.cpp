#include "histogram.hpp"
#include "support.hpp"

namespace histogram {
void serial(const Input& input, Counts& counts, Execution& execution) {
    const double begin = lab::now();
    for (auto bucket : input) ++counts[bucket];
    execution.threads = 1;
    auto& stat = execution.stats[0];
    stat.updates = input.size();
    stat.compute_s = lab::now() - begin;
}
}
