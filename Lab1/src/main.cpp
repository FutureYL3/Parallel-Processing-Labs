#include "benchmark.hpp"
#include <iostream>

int main(int argc, char** argv) {
    spmm::Config cfg;
    for (int i = 1; i < argc; ++i) {
        const std::string key = argv[i];
        if (key == "--help") {
            std::cout << "CSR SpMM: C[M,N] = A[M,K] * B[K,N]\n"
                         "--m 4096 --k 8192 --n 1024 --nnz-per-row 32 --seed 2026\n"
                         "--profile uniform|skewed|blocked --kernel serial|rows|balanced|tiled\n"
                         "--threads 4 --repeat 7 --schedule static|dynamic|guided --chunk 0\n"
                         "--block-nnz 256 --tile 128\n";
            return 0;
        }
        if (key == "--m") cfg.m = std::stoull(argv[++i]);
        else if (key == "--k") cfg.k = std::stoull(argv[++i]);
        else if (key == "--n") cfg.n = std::stoull(argv[++i]);
        else if (key == "--nnz-per-row") cfg.nnz_per_row = std::stoull(argv[++i]);
        else if (key == "--block-nnz") cfg.block_nnz = std::stoull(argv[++i]);
        else if (key == "--tile") cfg.tile = std::stoull(argv[++i]);
        else if (key == "--seed") cfg.seed = std::stoul(argv[++i]);
        else if (key == "--threads") cfg.threads = std::stoi(argv[++i]);
        else if (key == "--repeat") cfg.repeat = std::stoi(argv[++i]);
        else if (key == "--chunk") cfg.chunk = std::stoi(argv[++i]);
        else if (key == "--profile") cfg.profile = argv[++i];
        else if (key == "--kernel") cfg.kernel = argv[++i];
        else if (key == "--schedule") cfg.schedule = argv[++i];
    }
    spmm::benchmark(cfg);
    return 0;
}
