#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace histogram {
using Input = std::vector<std::uint32_t>;
using Counts = std::vector<std::uint64_t>;
Input generate_input(std::size_t n, std::size_t bins, const std::string& distribution,
                     std::uint32_t seed, int hot_percent);
}
