#include "dram.hpp"
#include "support.hpp"

namespace gemm {
SoftwareDram::SoftwareDram(Memory& memory, const Config& config, std::uint32_t seed)
    : memory_(memory), cfg_(config), rng_(seed) {}
bool SoftwareDram::available() {
    rng_ = lab::mix(rng_+0x9e3779b9u);
    return rng_%100 >= cfg_.stall_percent;
}
void SoftwareDram::drive(Vmatrix_accelerator& top, std::uint64_t cycle) {
    if (!r_valid_ && !reads_.empty() && cycle >= reads_.front().ready_cycle && cycle >= next_read_
        && (reads_.front().beat != 0 || available())) {
        r_data_ = memory_.read64(reads_.front().address);
        r_valid_ = true;
    }
    top.ar_ready = reads_.size()<2 && available();
    top.r_valid = r_valid_; top.r_data = r_data_;
    top.aw_ready = !write_active_ && !response_pending_ && available();
    top.w_ready = write_active_ && cycle>=next_write_ && (write_beat_!=0 || available());
    top.b_valid = response_pending_ && cycle>=response_cycle_;
}
BusSample SoftwareDram::sample(const Vmatrix_accelerator& top) const {
    return {bool(top.ar_valid && top.ar_ready), bool(top.r_valid && top.r_ready),
            bool(top.aw_valid && top.aw_ready), bool(top.w_valid && top.w_ready), bool(top.b_valid && top.b_ready),
            bool(top.compute_active), top.ar_address, top.ar_beats, top.aw_address, top.aw_beats,
            top.w_data, static_cast<std::uint8_t>(top.w_strobe)};
}
void SoftwareDram::update(const BusSample& s, std::uint64_t cycle) {
    if (s.read_data) {
        auto& read = reads_.front();
        ++read.beat; read.address += 8; r_valid_ = false; next_read_ = cycle+cfg_.beat_cycles;
        stats.read_bytes += 8;
        if (s.compute) ++stats.overlap_read_beats;
        if (read.beat == read.beats) reads_.pop_front();
    }
    if (s.read_address) {
        reads_.push_back({s.ar_address, s.ar_beats, 0, cycle+cfg_.latency});
        ++stats.read_requests;
    }
    if (s.write_address) {
        write_active_ = true; write_address_ = s.aw_address; write_beats_ = s.aw_beats; write_beat_ = 0;
        next_write_ = cycle+1; ++stats.write_requests;
    }
    if (s.write_data) {
        memory_.write64(write_address_, s.w_data, s.w_strobe);
        for (unsigned byte = 0; byte < 8; ++byte) stats.write_bytes += (s.w_strobe >> byte) & 1u;
        ++write_beat_; write_address_ += 8; next_write_ = cycle+cfg_.beat_cycles;
        if (write_beat_ == write_beats_) {
            write_active_ = false; response_pending_ = true; response_cycle_ = cycle+cfg_.latency;
        }
    }
    if (s.write_response) response_pending_ = false;
}
}
