#pragma once
#include "communication.hpp"

namespace spmm {
struct RankStats {
    std::uint64_t row_begin = 0, row_end = 0, nnz = 0, local_nnz = 0, remote_nnz = 0;
    std::uint64_t send_bytes = 0, receive_bytes = 0;
    double compute_s = 0, pack_s = 0, mpi_s = 0, wait_s = 0, rank_elapsed_s = 0;
};

// The same row-block / column-panel / 32-column microkernel is used everywhere.
void multiply_tiled(const CsrMatrix&, const DenseMatrix&, DenseMatrix&,
                     const std::vector<RowBlock>&, std::size_t tile, bool add = false);
void serial(const CsrMatrix&, DenseMatrix&, Workspace&, const Config&, RankStats&);
void broadcast(const CsrMatrix&, DenseMatrix&, Workspace&, const Config&,
                const Distribution&, MPI_Comm, RankStats&);
void allgather(const CsrMatrix&, const DenseMatrix&, DenseMatrix&, Workspace&,
               const Config&, const Distribution&, MPI_Comm, RankStats&);
void halo(const DenseMatrix&, DenseMatrix&, Workspace&, const Config&,
           const Distribution&, MPI_Comm, RankStats&);
void overlap(const DenseMatrix&, DenseMatrix&, Workspace&, const Config&,
              const Distribution&, MPI_Comm, RankStats&);
}
