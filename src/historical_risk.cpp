
#include "historical_risk.hpp"

#include "returns.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <vector>

namespace ql {

HistoricalRiskResult HistoricalRisk::calculate(
    const std::vector<double>& prices,
    double confidenceLevel,
    HistoricalQuantileConvention convention
) const {
    const std::vector<double> returns = historicalReturns(prices);

    return calculateFromReturns(
        returns,
        confidenceLevel,
        convention
    );
}

HistoricalRiskResult HistoricalRisk::calculateFromReturns(
    const std::vector<double>& returns,
    double confidenceLevel,
    HistoricalQuantileConvention convention
) const {
    if (returns.empty()) {
        throw std::invalid_argument(
            "Returns series must not be empty."
        );
    }

    if (!std::isfinite(confidenceLevel) ||
        confidenceLevel <= 0.0 ||
        confidenceLevel >= 1.0) {
        throw std::invalid_argument(
            "Confidence level must be between 0 and 1."
        );
    }

    for (double value : returns) {
        if (!std::isfinite(value)) {
            throw std::invalid_argument(
                "Returns series contains a non-finite value."
            );
        }
    }

    HistoricalVaR varCalculator;

    const double var = varCalculator.calculate(
        returns,
        confidenceLevel,
        convention
    );

    std::vector<double> sortedReturns = returns;

    std::sort(
        sortedReturns.begin(),
        sortedReturns.end()
    );

    // Fractional-tail ES: average the worst (1 - alpha) * N
    // observations, allowing a fractional boundary observation.
    const double tailSize =
        (1.0 - confidenceLevel) *
        static_cast<double>(sortedReturns.size());

    if (!std::isfinite(tailSize) || tailSize <= 0.0) {
        throw std::invalid_argument(
            "Expected Shortfall tail must be positive."
        );
    }

    const auto fullTailCount =
        static_cast<std::size_t>(std::floor(tailSize));

    double tailLoss = 0.0;

    for (std::size_t i = 0; i < fullTailCount; ++i) {
        tailLoss += -sortedReturns[i];
    }

    const double fractionalPart =
        tailSize - static_cast<double>(fullTailCount);

    if (fractionalPart > 0.0 &&
        fullTailCount < sortedReturns.size()) {
        tailLoss +=
            fractionalPart * (-sortedReturns[fullTailCount]);
    }

    const double expectedShortfall =
        std::max(0.0, tailLoss / tailSize);

    if (!std::isfinite(var) ||
        !std::isfinite(expectedShortfall)) {
        throw std::overflow_error(
            "Historical risk calculation produced a non-finite result."
        );
    }

    return {
        returns,
        var,
        expectedShortfall
    };
}

} // namespace ql
