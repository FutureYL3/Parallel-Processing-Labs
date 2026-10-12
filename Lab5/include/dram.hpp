#pragma once
#include "matrix.hpp"
#include "Vmatrix_accelerator.h"
#include <deque>

namespace gemm {
struct BusSample {
    bool read_address, read_data, write_address, write_data, write_response, compute;
    std::uint32_t ar_address, ar_beats, aw_address, aw_beats;
    std::uint64_t w_data;
    std::uint8_t w_strobe;
};
struct DramStats {
    std::uint64_t read_requests = 0, write_requests = 0, read_bytes = 0, write_bytes = 0;
    std::uint64_t overlap_read_beats = 0;
};
class SoftwareDram {
public:
    SoftwareDram(Memory& memory, const Config& config, std::uint32_t seed);
    void drive(Vmatrix_accelerator&, std::uint64_t cycle);
    BusSample sample(const Vmatrix_accelerator&) const;
    void update(const BusSample&, std::uint64_t cycle);
    DramStats stats;
private:
    struct Read { std::uint32_t address, beats, beat = 0; std::uint64_t ready_cycle; };
    Memory& memory_;
    const Config& cfg_;
    std::uint32_t rng_;
    std::deque<Read> reads_;
    bool r_valid_ = false, write_active_ = false, response_pending_ = false;
    std::uint64_t r_data_ = 0, next_read_ = 0, next_write_ = 0, response_cycle_ = 0;
    std::uint32_t write_address_ = 0, write_beats_ = 0, write_beat_ = 0;
    bool available();
};
}
