
#include "historical_risk.hpp"

#include "historical_var.hpp"
#include "returns.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>

namespace ql {

HistoricalRiskResult HistoricalRisk::calculate(
    const std::vector<double>& prices,
    double confidenceLevel
) const {
    const std::vector<double> returns = historicalReturns(prices);

    return calculateFromReturns(returns, confidenceLevel);
}

HistoricalRiskResult HistoricalRisk::calculateFromReturns(
    const std::vector<double>& returns,
    double confidenceLevel
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

    // Calculate Historical VaR.
    HistoricalVaR varCalculator;

    const double var =
        varCalculator.calculate(returns, confidenceLevel);

    // Sort returns from worst to best.
    std::vector<double> sortedReturns = returns;

    std::sort(sortedReturns.begin(), sortedReturns.end());

    // Number of observations in the worst (1 - confidence) tail.
    const double tailSize =
        (1.0 - confidenceLevel) *
        static_cast<double>(sortedReturns.size());

    const auto fullTailCount =
        static_cast<std::size_t>(std::floor(tailSize));

    double tailLoss = 0.0;

    for (std::size_t i = 0; i < fullTailCount; ++i) {
        tailLoss += -sortedReturns[i];
    }

    // Include a fractional observation at the tail boundary.
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
