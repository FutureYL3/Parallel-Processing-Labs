#include "benchmark.hpp"
#include <iostream>

int main(int argc, char** argv) {
    histogram::Config cfg;
    for (int i = 1; i < argc; ++i) {
        const std::string key = argv[i];
        if (key == "--help") {
            std::cout << "Integer histogram: counts[input[i]] += 1\n"
                         "--n 10000000 --bins 256 --stripes 16 --seed 2026\n"
                         "--kernel serial|critical|atomic|locks|private\n"
                         "--distribution uniform|hotspot --hot-percent 90\n"
                         "--threads 4 --repeat 7\n";
            return 0;
        }
        if (key == "--n") cfg.n = std::stoull(argv[++i]);
        else if (key == "--bins") cfg.bins = std::stoull(argv[++i]);
        else if (key == "--stripes") cfg.stripes = std::stoull(argv[++i]);
        else if (key == "--threads") cfg.threads = std::stoi(argv[++i]);
        else if (key == "--repeat") cfg.repeat = std::stoi(argv[++i]);
        else if (key == "--hot-percent") cfg.hot_percent = std::stoi(argv[++i]);
        else if (key == "--seed") cfg.seed = std::stoul(argv[++i]);
        else if (key == "--distribution") cfg.distribution = argv[++i];
        else if (key == "--kernel") cfg.kernel = argv[++i];
    }
    histogram::benchmark(cfg);
    return 0;
}
