
#include "historical_var.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <vector>

namespace ql {

double HistoricalVaR::calculate(
    const std::vector<double>& returns,
    double confidenceLevel,
    HistoricalQuantileConvention convention
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

    const std::size_t sampleSize = sortedReturns.size();

    double quantile = 0.0;

    switch (convention) {
    case HistoricalQuantileConvention::LinearInterpolation: {
        // h = (1 - confidenceLevel) * (N - 1)
        // Interpolate between the adjacent sorted observations.
        const double position =
            (1.0 - confidenceLevel) *
            static_cast<double>(sampleSize - 1);

        const auto lowerIndex =
            static_cast<std::size_t>(std::floor(position));

        const auto upperIndex =
            static_cast<std::size_t>(std::ceil(position));

        const double fraction =
            position - static_cast<double>(lowerIndex);

        quantile =
            sortedReturns[lowerIndex] +
            fraction *
                (sortedReturns[upperIndex] -
                 sortedReturns[lowerIndex]);

        break;
    }

    case HistoricalQuantileConvention::NearestRank: {
        // Lower-tail rank = ceil((1 - confidenceLevel) * N).
        // Convert the one-based rank to a zero-based index.
        const double tailSize =
            (1.0 - confidenceLevel) *
            static_cast<double>(sampleSize);

        const auto rank = static_cast<std::size_t>(
            std::ceil(tailSize)
        );

        const std::size_t index =
            std::max<std::size_t>(1, rank) - 1;

        quantile = sortedReturns[index];

        break;
    }

    default:
        throw std::invalid_argument(
            "Unknown historical quantile convention"
        );
    }

    // Return VaR as a non-negative loss fraction.
    return std::max(0.0, -quantile);
}

} // namespace ql
