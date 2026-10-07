#include "historical_var.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>

namespace ql {

double HistoricalVaR::calculate(
    const std::vector<double>& returns,
    double confidenceLevel
) const {

    if (returns.empty()) {
        throw std::invalid_argument(
            "Returns series must not be empty"
        );
    }

    if (!std::isfinite(confidenceLevel) ||
        confidenceLevel <= 0.0 ||
        confidenceLevel >= 1.0) {
        throw std::invalid_argument(
            "Confidence level must be between 0 and 1"
        );
    }

    for (double value : returns) {
        if (!std::isfinite(value)) {
            throw std::invalid_argument(
                "Returns series contains a non-finite value"
            );
        }
    }

    std::vector<double> sortedReturns = returns;

    std::sort(
        sortedReturns.begin(),
        sortedReturns.end()
    );

    /*
     * Historical VaR at confidence level alpha:
     *
     *     VaR = - quantile(1 - alpha)
     *
     * Linear interpolation is used between observations.
     */

    const double position =
        (1.0 - confidenceLevel) *
        static_cast<double>(sortedReturns.size() - 1);

    const auto lowerIndex =
        static_cast<std::size_t>(std::floor(position));

    const auto upperIndex =
        static_cast<std::size_t>(std::ceil(position));

    const double fraction =
        position - static_cast<double>(lowerIndex);

    const double quantile =
        sortedReturns[lowerIndex] +
        fraction *
        (sortedReturns[upperIndex] -
         sortedReturns[lowerIndex]);

    return std::max(0.0, -quantile);
}

} // namespace ql