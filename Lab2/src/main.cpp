#include "benchmark.hpp"
#include <iostream>

int main(int argc, char** argv) {
    transpose::Config cfg;
    for (int i = 1; i < argc; ++i) {
        const std::string key = argv[i];
        if (key == "--help") {
            std::cout << "Matrix transpose: B[j,i] = A[i,j]\n"
                         "--n 2048 --tile 32 --seed 2026\n"
                         "--kernel serial|naive|tiled --threads 4 --repeat 7\n";
            return 0;
        }
        if (key == "--n") cfg.n = std::stoull(argv[++i]);
        else if (key == "--tile") cfg.tile = std::stoull(argv[++i]);
        else if (key == "--threads") cfg.threads = std::stoi(argv[++i]);
        else if (key == "--repeat") cfg.repeat = std::stoi(argv[++i]);
        else if (key == "--seed") cfg.seed = std::stoul(argv[++i]);
        else if (key == "--kernel") cfg.kernel = argv[++i];
    }
    transpose::benchmark(cfg);
    return 0;
}
