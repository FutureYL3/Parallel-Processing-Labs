#pragma once
#include "config.hpp"
#include <cstddef>
#include <vector>

namespace gemm {
class Memory {
public:
    explicit Memory(std::size_t bytes) : bytes_(bytes, 0) {}
    std::uint64_t read64(std::uint32_t address) const;
    std::int32_t read32(std::uint32_t address) const;
    void write64(std::uint32_t address, std::uint64_t data, std::uint8_t strobe);
    void write16(std::uint32_t address, std::int16_t data);
private:
    std::vector<std::uint8_t> bytes_;
};
struct Matrices {
    std::uint32_t a_base = 0, b_base, c_base;
    Memory memory;
    explicit Matrices(const Config&);
    std::vector<std::int32_t> output(const Config&) const;
};
}
