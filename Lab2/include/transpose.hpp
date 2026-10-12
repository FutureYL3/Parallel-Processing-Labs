#pragma once
#include "matrix.hpp"

namespace transpose {
struct ThreadStats {
    std::uint64_t row_visits = 0, tile_visits = 0, updates = 0;
    double compute_s = 0.0, wait_s = 0.0;
};
struct Execution {
    int threads = 1;
    std::vector<ThreadStats> stats;
};
void serial(const Matrix&, Matrix&, Execution&);
void parallel_naive(const Matrix&, Matrix&, Execution&);
void parallel_tiled(const Matrix&, Matrix&, std::size_t tile, Execution&);
}
