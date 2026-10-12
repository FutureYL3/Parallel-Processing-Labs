#include "transpose.hpp"
#include "support.hpp"

namespace transpose {
void serial(const Matrix& input, Matrix& output, Execution& execution) {
    const auto n = input.n;
    const double begin = lab::now();
    for (std::size_t i = 0; i < n; ++i)
        for (std::size_t j = 0; j < n; ++j)
            output.values[j * n + i] = input.values[i * n + j];
    execution.threads = 1;
    auto& stat = execution.stats[0];
    stat.row_visits = n;
    stat.updates = n * n;
    stat.compute_s = lab::now() - begin;
}
}
