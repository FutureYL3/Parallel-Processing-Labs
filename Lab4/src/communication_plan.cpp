#include "communication.hpp"
#include <algorithm>
#include <numeric>

namespace spmm {
namespace {
std::vector<int> displacements(const std::vector<int>& counts) {
    std::vector<int> offsets(counts.size(), 0);
    for (std::size_t p = 1; p < counts.size(); ++p) offsets[p] = offsets[p - 1] + counts[p - 1];
    return offsets;
}

HaloPlan make_halo_plan(const CsrMatrix& a, const DenseMatrix& b,
                        const Distribution& d, MPI_Comm comm) {
    HaloPlan h;
    auto columns = a.col_idx;
    std::sort(columns.begin(), columns.end());
    columns.erase(std::unique(columns.begin(), columns.end()), columns.end());
    std::vector<std::uint32_t> local_rows, requested_rows;
    std::vector<int> receive_rows(d.ranks, 0), send_rows(d.ranks, 0);
    for (auto column : columns) {
        if (column >= d.b_begin && column < d.b_end) local_rows.push_back(column);
        else {
            const auto owner = static_cast<int>(std::upper_bound(d.b_offsets.begin(), d.b_offsets.end(), column) - d.b_offsets.begin() - 1);
            ++receive_rows[owner]; requested_rows.push_back(column);
        }
    }
    const auto receive_offsets = displacements(receive_rows);
    MPI_Alltoall(receive_rows.data(), 1, MPI_INT, send_rows.data(), 1, MPI_INT, comm);
    const auto send_offsets = displacements(send_rows);
    h.send_rows.resize(std::accumulate(send_rows.begin(), send_rows.end(), 0));
    MPI_Alltoallv(requested_rows.data(), receive_rows.data(), receive_offsets.data(), MPI_UINT32_T,
                  h.send_rows.data(), send_rows.data(), send_offsets.data(), MPI_UINT32_T, comm);

    h.local_b_rows = local_rows.size();
    h.local_rows = local_rows;
    h.needed_b = DenseMatrix(columns.size(), b.cols);
    std::vector<std::uint32_t> slot(a.cols, 0);
    for (std::size_t i = 0; i < local_rows.size(); ++i) {
        slot[local_rows[i]] = static_cast<std::uint32_t>(i);
        const auto* input = b.values.data() + (local_rows[i] - d.b_begin) * b.cols;
        std::copy_n(input, b.cols, h.needed_b.values.data() + i * b.cols);
    }
    for (std::size_t i = 0; i < requested_rows.size(); ++i)
        slot[requested_rows[i]] = static_cast<std::uint32_t>(local_rows.size() + i);

    h.all = a; h.all.cols = columns.size();
    h.local.rows = h.remote.rows = a.rows;
    h.local.cols = h.remote.cols = columns.size();
    h.local.row_ptr.push_back(0); h.remote.row_ptr.push_back(0);
    for (std::size_t row = 0; row < a.rows; ++row) {
        for (std::size_t p = a.row_ptr[row]; p < a.row_ptr[row + 1]; ++p) {
            const auto column = a.col_idx[p];
            h.all.col_idx[p] = slot[column];
            auto& part = column >= d.b_begin && column < d.b_end ? h.local : h.remote;
            part.col_idx.push_back(slot[column]); part.values.push_back(a.values[p]);
        }
        h.local.row_ptr.push_back(h.local.values.size());
        h.remote.row_ptr.push_back(h.remote.values.size());
    }
    h.send_counts.resize(d.ranks); h.receive_counts.resize(d.ranks);
    for (int p = 0; p < d.ranks; ++p) {
        h.send_counts[p] = send_rows[p] * static_cast<int>(b.cols);
        h.receive_counts[p] = receive_rows[p] * static_cast<int>(b.cols);
    }
    h.send_displacements = displacements(h.send_counts);
    h.receive_displacements = displacements(h.receive_counts);
    h.send_values.resize(h.send_rows.size() * b.cols);
    return h;
}
}

Workspace prepare_workspace(const Config& cfg, const Distribution& d, const CsrMatrix& a,
                             const DenseMatrix& b, MPI_Comm comm) {
    Workspace w;
    w.blocks = make_row_blocks(a, cfg.block_nnz);
    for (auto column : a.col_idx)
        if (column >= d.b_begin && column < d.b_end) ++w.local_nnz;
    w.remote_nnz = a.values.size() - w.local_nnz;
    if (cfg.kernel == "halo" || cfg.kernel == "overlap") {
        w.halo = make_halo_plan(a, b, d, comm);
        w.send_bytes = w.halo.send_values.size() * sizeof(double);
        w.receive_bytes = (w.halo.needed_b.rows - w.halo.local_b_rows) * cfg.n * sizeof(double);
    } else {
        w.full_b = DenseMatrix(cfg.k, cfg.n);
        if (cfg.kernel == "serial" || (cfg.kernel == "broadcast" && d.rank == 0))
            w.full_b = generate_dense_rows(0, cfg.k, cfg.n, cfg.seed + 1);
        if (cfg.kernel == "broadcast") {
            w.send_bytes = d.rank == 0 ? cfg.k * cfg.n * sizeof(double) * (d.ranks - 1) : 0;
            w.receive_bytes = d.rank == 0 ? 0 : cfg.k * cfg.n * sizeof(double);
        } else if (cfg.kernel == "allgather") {
            w.send_bytes = b.values.size() * sizeof(double) * (d.ranks - 1);
            w.receive_bytes = (cfg.k - b.rows) * cfg.n * sizeof(double);
        }
    }
    return w;
}

void pack_send_rows(HaloPlan& h, const DenseMatrix& b, const Distribution& d) {
    for (std::size_t i = 0; i < h.local_rows.size(); ++i)
        std::copy_n(b.values.data() + (h.local_rows[i] - d.b_begin) * b.cols,
                    b.cols, h.needed_b.values.data() + i * b.cols);
    for (std::size_t i = 0; i < h.send_rows.size(); ++i)
        std::copy_n(b.values.data() + (h.send_rows[i] - d.b_begin) * b.cols,
                    b.cols, h.send_values.data() + i * b.cols);
}
}
