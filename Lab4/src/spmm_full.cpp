#include "spmm.hpp"

namespace spmm {
void serial(const CsrMatrix& a, DenseMatrix& c, Workspace& w,
             const Config& cfg, RankStats& stat) {
    const auto start = MPI_Wtime();
    multiply_tiled(a, w.full_b, c, w.blocks, cfg.tile);
    stat.compute_s = MPI_Wtime() - start;
}

void broadcast(const CsrMatrix& a, DenseMatrix& c, Workspace& w, const Config& cfg,
                const Distribution&, MPI_Comm comm, RankStats& stat) {
    auto start = MPI_Wtime();
    MPI_Bcast(w.full_b.values.data(), static_cast<int>(w.full_b.values.size()), MPI_DOUBLE, 0, comm);
    stat.mpi_s = MPI_Wtime() - start;
    start = MPI_Wtime();
    multiply_tiled(a, w.full_b, c, w.blocks, cfg.tile);
    stat.compute_s = MPI_Wtime() - start;
}

void allgather(const CsrMatrix& a, const DenseMatrix& b, DenseMatrix& c, Workspace& w,
               const Config& cfg, const Distribution& d, MPI_Comm comm, RankStats& stat) {
    auto start = MPI_Wtime();
    MPI_Allgatherv(b.values.data(), static_cast<int>(b.values.size()), MPI_DOUBLE,
                   w.full_b.values.data(), d.b_counts.data(), d.b_displacements.data(), MPI_DOUBLE, comm);
    stat.mpi_s = MPI_Wtime() - start;
    start = MPI_Wtime();
    multiply_tiled(a, w.full_b, c, w.blocks, cfg.tile);
    stat.compute_s = MPI_Wtime() - start;
}
}
