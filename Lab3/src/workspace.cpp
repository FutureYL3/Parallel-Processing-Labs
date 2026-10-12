#include "histogram.hpp"

namespace histogram {
Workspace::Workspace(std::size_t bins, int threads, std::size_t stripes, const std::string& kernel)
    : stride(((bins + 15) / 16) * 16 + 16),
      local_counts(kernel == "private" ? stride * threads : 0),
      locks(kernel == "locks" ? stripes : 0) {
    for (auto& lock : locks) omp_init_lock(&lock);
}

Workspace::~Workspace() {
    for (auto& lock : locks) omp_destroy_lock(&lock);
}
}
