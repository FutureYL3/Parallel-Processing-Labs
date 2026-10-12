#pragma once
#include "input.hpp"
#include <omp.h>

namespace histogram {
struct ThreadStats {
    std::uint64_t updates = 0, merged_bins = 0;
    double compute_s = 0.0, wait_s = 0.0, merge_s = 0.0;
};
struct Execution {
    int threads = 1;
    std::vector<ThreadStats> stats;
};
struct Workspace {
    std::size_t stride;
    std::vector<std::uint64_t> local_counts;
    std::vector<omp_lock_t> locks;
    Workspace(std::size_t bins, int threads, std::size_t stripes, const std::string& kernel);
    ~Workspace();
    Workspace(const Workspace&) = delete;
    Workspace& operator=(const Workspace&) = delete;
};

void serial(const Input&, Counts&, Execution&);
void critical(const Input&, Counts&, Execution&);
void atomic(const Input&, Counts&, Execution&);
void locks(const Input&, Counts&, Workspace&, Execution&);
void private_counts(const Input&, Counts&, Workspace&, Execution&);
}
