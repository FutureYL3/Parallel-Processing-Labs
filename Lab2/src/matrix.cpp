#include "matrix.hpp"
#include "support.hpp"

namespace transpose {
Matrix::Matrix(std::size_t size) : n(size), values(size * size) {}

Matrix generate_matrix(std::size_t n, std::uint32_t seed) {
    Matrix matrix(n);
    for (std::size_t i = 0; i < matrix.values.size(); ++i)
        matrix.values[i] = lab::mix(static_cast<std::uint32_t>(i) + seed);
    return matrix;
}
}
