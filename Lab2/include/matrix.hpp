#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

namespace transpose {
struct Matrix {
    std::size_t n;
    std::vector<std::uint32_t> values;
    explicit Matrix(std::size_t size);
};
Matrix generate_matrix(std::size_t n, std::uint32_t seed);
}
