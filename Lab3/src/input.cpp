#include "input.hpp"
#include "support.hpp"

namespace histogram {
Input generate_input(std::size_t n, std::size_t bins, const std::string& distribution,
                     std::uint32_t seed, int hot_percent) {
    Input input(n);
    for (std::size_t i = 0; i < n; ++i) {
        const auto value = lab::mix(static_cast<std::uint32_t>(i) + seed);
        const bool hot = distribution == "hotspot" && value % 100u < static_cast<unsigned>(hot_percent);
        input[i] = hot ? 0u : static_cast<std::uint32_t>(lab::mix(value) % bins);
    }
    return input;
}
}
