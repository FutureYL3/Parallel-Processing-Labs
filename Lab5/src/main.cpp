#include "simulator.hpp"

int main(int argc,char** argv) {
    gemm::Config cfg;
    for (int i=1;i<argc;++i) {
        const std::string option=argv[i], value=argv[++i];
        if (option=="--m") cfg.m=std::stoul(value);
        else if (option=="--k") cfg.k=std::stoul(value);
        else if (option=="--n") cfg.n=std::stoul(value);
        else if (option=="--tile-m") cfg.tile_m=std::stoul(value);
        else if (option=="--tile-k") cfg.tile_k=std::stoul(value);
        else if (option=="--tile-n") cfg.tile_n=std::stoul(value);
        else if (option=="--latency") cfg.latency=std::stoul(value);
        else if (option=="--beat-cycles") cfg.beat_cycles=std::stoul(value);
        else if (option=="--stall-percent") cfg.stall_percent=std::stoul(value);
        else if (option=="--seed") cfg.seed=std::stoul(value);
        else if (option=="--repeat") cfg.repeat=std::stoi(value);
        else if (option=="--buffering") cfg.double_buffer=value=="double";
        else if (option=="--trace") cfg.trace=value;
        else if (option=="--trace-start") cfg.trace_start=std::stoull(value);
        else if (option=="--trace-cycles") cfg.trace_cycles=std::stoull(value);
        else if (option=="--dump-output") cfg.dump_output=value;
    }
    gemm::benchmark(cfg);
}
