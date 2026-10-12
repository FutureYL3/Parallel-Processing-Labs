#include "spmm.hpp"

namespace spmm {
void overlap(const DenseMatrix& b, DenseMatrix& c, Workspace& w, const Config& cfg,
              const Distribution& d, MPI_Comm comm, RankStats& stat) {
    auto& h = w.halo;
    auto start = MPI_Wtime();
    pack_send_rows(h, b, d);
    stat.pack_s = MPI_Wtime() - start;
    MPI_Request request;
    start = MPI_Wtime();
    // Local entries read only the local prefix of needed_b. The receive writes
    // its remote suffix, which is not read until the request has completed.
    MPI_Ialltoallv(h.send_values.data(), h.send_counts.data(), h.send_displacements.data(), MPI_DOUBLE,
                   h.needed_b.values.data() + h.local_b_rows * cfg.n,
                   h.receive_counts.data(), h.receive_displacements.data(), MPI_DOUBLE, comm, &request);
    stat.mpi_s = MPI_Wtime() - start;
    start = MPI_Wtime();
    multiply_tiled(h.local, h.needed_b, c, w.blocks, cfg.tile);
    stat.compute_s = MPI_Wtime() - start;
    start = MPI_Wtime();
    MPI_Wait(&request, MPI_STATUS_IGNORE);
    stat.wait_s = MPI_Wtime() - start;
    stat.mpi_s += stat.wait_s;
    if (!h.remote.values.empty()) {
        start = MPI_Wtime();
        multiply_tiled(h.remote, h.needed_b, c, w.blocks, cfg.tile, true);
        stat.compute_s += MPI_Wtime() - start;
    }
}
}
