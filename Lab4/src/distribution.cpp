#include "distribution.hpp"
#include <algorithm>

namespace spmm {
Distribution make_distribution(const Config& cfg, MPI_Comm comm) {
    Distribution d;
    MPI_Comm_rank(comm, &d.rank); MPI_Comm_size(comm, &d.ranks);
    d.a_offsets.resize(d.ranks + 1); d.b_offsets.resize(d.ranks + 1);
    d.b_counts.resize(d.ranks); d.b_displacements.resize(d.ranks);
    std::vector<std::size_t> prefix(cfg.m + 1, 0);
    if (cfg.partition == "nnz")
        for (std::size_t i = 0; i < cfg.m; ++i)
            prefix[i + 1] = prefix[i] + row_nonzeros(i, cfg.m, cfg.k, cfg.nnz_per_row, cfg.profile);
    for (int p = 0; p <= d.ranks; ++p) {
        d.a_offsets[p] = cfg.partition == "rows" ? cfg.m * p / d.ranks
            : static_cast<std::size_t>(std::lower_bound(prefix.begin(), prefix.end(), prefix.back() * p / d.ranks) - prefix.begin());
        d.b_offsets[p] = cfg.k * p / d.ranks;
    }
    d.a_offsets.back() = cfg.m;
    for (int p = 0; p < d.ranks; ++p) {
        d.b_counts[p] = static_cast<int>((d.b_offsets[p + 1] - d.b_offsets[p]) * cfg.n);
        d.b_displacements[p] = static_cast<int>(d.b_offsets[p] * cfg.n);
    }
    d.row_begin = d.a_offsets[d.rank]; d.row_end = d.a_offsets[d.rank + 1];
    d.b_begin = d.b_offsets[d.rank]; d.b_end = d.b_offsets[d.rank + 1];
    return d;
}
}
