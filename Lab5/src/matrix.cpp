#include "matrix.hpp"
#include "support.hpp"

namespace gemm {
namespace {
std::uint32_t align64(std::uint32_t bytes) { return (bytes+63)/64*64; }
}
std::uint64_t Memory::read64(std::uint32_t address) const {
    std::uint64_t value = 0;
    for (unsigned byte = 0; byte < 8; ++byte) value |= std::uint64_t(bytes_[address+byte]) << (byte*8);
    return value;
}
std::int32_t Memory::read32(std::uint32_t address) const {
    return static_cast<std::int32_t>(static_cast<std::uint32_t>(read64(address)));
}
void Memory::write64(std::uint32_t address, std::uint64_t data, std::uint8_t strobe) {
    for (unsigned byte = 0; byte < 8; ++byte)
        if ((strobe >> byte) & 1u) bytes_[address+byte] = static_cast<std::uint8_t>(data >> (byte*8));
}
void Memory::write16(std::uint32_t address, std::int16_t data) {
    bytes_[address] = static_cast<std::uint8_t>(data);
    bytes_[address+1] = static_cast<std::uint8_t>(static_cast<std::uint16_t>(data) >> 8);
}
Matrices::Matrices(const Config& cfg)
    : b_base(align64(cfg.m*cfg.k*2)), c_base(b_base+align64(cfg.k*cfg.n*2)),
      memory(c_base+align64(cfg.m*cfg.n*4)+64) {
    for (std::uint32_t i = 0; i < cfg.m*cfg.k; ++i)
        memory.write16(a_base+i*2, static_cast<std::int16_t>(int(lab::mix(i+cfg.seed)%7)-3));
    for (std::uint32_t i = 0; i < cfg.k*cfg.n; ++i)
        memory.write16(b_base+i*2, static_cast<std::int16_t>(int(lab::mix(i+cfg.seed+1)%7)-3));
}
std::vector<std::int32_t> Matrices::output(const Config& cfg) const {
    std::vector<std::int32_t> result(cfg.m*cfg.n);
    for (std::uint32_t i = 0; i < result.size(); ++i) result[i] = memory.read32(c_base+i*4);
    return result;
}
}
