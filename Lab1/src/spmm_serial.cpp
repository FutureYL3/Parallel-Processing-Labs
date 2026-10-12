#include "detail/row_kernel.hpp"
#include "support.hpp"

namespace spmm {
void serial(const CsrMatrix& a, const DenseMatrix& b, DenseMatrix& c, Execution& execution) {
    execution.threads = 1;
    const double begin = lab::now();
    detail::accumulate(a, b, c, 0, a.rows, 0, b.cols, execution.stats[0]);
    execution.stats[0].compute_s = lab::now() - begin;
}
}
