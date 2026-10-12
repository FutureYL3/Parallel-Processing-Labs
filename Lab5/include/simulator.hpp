#pragma once
#include "dram.hpp"

namespace gemm {
struct Result {
    std::uint64_t cycles, compute_cycles, input_wait_cycles, output_cycles, overlap_cycles, macs;
    DramStats dram;
    std::int64_t checksum = 0;
    double host_seconds = 0;
    std::vector<std::int32_t> output;
};
Result simulate(const Config&, std::uint32_t timing_seed, bool save_trace);
void benchmark(const Config&);
}
