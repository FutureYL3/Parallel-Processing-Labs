#pragma once
#include "config.hpp"
#include "matrix.hpp"
#include <mpi.h>

namespace spmm {
struct Distribution {
    int rank = 0, ranks = 1;
    std::size_t row_begin = 0, row_end = 0, b_begin = 0, b_end = 0;
    std::vector<std::size_t> a_offsets, b_offsets;
    std::vector<int> b_counts, b_displacements;
};
Distribution make_distribution(const Config&, MPI_Comm);
}
