#pragma once
#include "distribution.hpp"

namespace spmm {
// Receive rows are deduplicated and grouped by their owning rank.
struct HaloPlan {
    CsrMatrix all, local, remote;
    DenseMatrix needed_b;
    std::size_t local_b_rows = 0;
    std::vector<std::uint32_t> local_rows, send_rows;
    std::vector<int> send_counts, send_displacements, receive_counts, receive_displacements;
    std::vector<double> send_values;
};
struct Workspace {
    DenseMatrix full_b;
    HaloPlan halo;
    std::vector<RowBlock> blocks;
    std::uint64_t local_nnz = 0, remote_nnz = 0, send_bytes = 0, receive_bytes = 0;
};
Workspace prepare_workspace(const Config&, const Distribution&, const CsrMatrix&,
                             const DenseMatrix&, MPI_Comm);
void pack_send_rows(HaloPlan&, const DenseMatrix&, const Distribution&);
}
