
#include "portfolio_weight_validation.hpp"

#include <cmath>
#include <stdexcept>

namespace ql {

void validatePortfolioWeights(const std::vector<double>& weights) {
    if (weights.empty()) {
        throw std::invalid_argument(
            "Portfolio weights must not be empty."
        );
    }

    long double sum = 0.0L;

    for (double weight : weights) {
        if (!std::isfinite(weight)) {
            throw std::invalid_argument(
                "Portfolio weights must all be finite."
            );
        }

        sum += static_cast<long double>(weight);
    }

    constexpr long double tolerance = 1e-8L;

    if (std::abs(sum - 1.0L) > tolerance) {
        throw std::invalid_argument(
            "Portfolio weights must sum to 1.0."
        );
    }
}

} // namespace ql
