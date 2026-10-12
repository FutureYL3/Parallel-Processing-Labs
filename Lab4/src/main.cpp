#include "benchmark.hpp"

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    spmm::Config cfg;
    for (int i = 1; i < argc; ++i) {
        const std::string option = argv[i];
        const std::string value = argv[++i];
        if (option == "--m") cfg.m = std::stoull(value);
        else if (option == "--k") cfg.k = std::stoull(value);
        else if (option == "--n") cfg.n = std::stoull(value);
        else if (option == "--nnz-per-row") cfg.nnz_per_row = std::stoull(value);
        else if (option == "--tile") cfg.tile = std::stoull(value);
        else if (option == "--block-nnz") cfg.block_nnz = std::stoull(value);
        else if (option == "--repeat") cfg.repeat = std::stoi(value);
        else if (option == "--seed") cfg.seed = static_cast<std::uint32_t>(std::stoul(value));
        else if (option == "--profile") cfg.profile = value;
        else if (option == "--kernel") cfg.kernel = value;
        else if (option == "--partition") cfg.partition = value;
    }
    spmm::benchmark(cfg, MPI_COMM_WORLD);
    MPI_Finalize();
}
