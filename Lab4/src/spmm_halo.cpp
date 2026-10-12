#include "spmm.hpp"

namespace spmm {
void halo(const DenseMatrix& b, DenseMatrix& c, Workspace& w, const Config& cfg,
           const Distribution& d, MPI_Comm comm, RankStats& stat) {
    auto& h = w.halo;
    auto start = MPI_Wtime();
    pack_send_rows(h, b, d);
    stat.pack_s = MPI_Wtime() - start;
    start = MPI_Wtime();
    MPI_Alltoallv(h.send_values.data(), h.send_counts.data(), h.send_displacements.data(), MPI_DOUBLE,
                  h.needed_b.values.data() + h.local_b_rows * cfg.n,
                  h.receive_counts.data(), h.receive_displacements.data(), MPI_DOUBLE, comm);
    stat.mpi_s = MPI_Wtime() - start;
    start = MPI_Wtime();
    multiply_tiled(h.all, h.needed_b, c, w.blocks, cfg.tile);
    stat.compute_s = MPI_Wtime() - start;
}
}
