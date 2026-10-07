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
    if (!std::isfinite(confidenceLevel) ||
        confidenceLevel <= 0.0 ||
        confidenceLevel >= 1.0) {
        throw std::invalid_argument(
            "Confidence level must be between 0 and 1"
        );
    }

    // Step 1: Convert prices into historical returns.
    const std::vector<double> returns =
        historicalReturns(prices);

    // Step 2: Calculate Historical VaR.
    HistoricalVaR varCalculator;

    const double var =
        varCalculator.calculate(returns, confidenceLevel);

    // Step 3: Sort returns from worst to best.
    std::vector<double> sortedReturns = returns;

    std::sort(
        sortedReturns.begin(),
        sortedReturns.end()
    );

    // Size of the worst (1 - confidenceLevel) tail.
    const double tailSize =
        (1.0 - confidenceLevel) *
        static_cast<double>(sortedReturns.size());

    double tailLoss = 0.0;

    const auto fullTailCount =
        static_cast<std::size_t>(std::floor(tailSize));

    // Add complete observations from the worst tail.
    for (std::size_t i = 0; i < fullTailCount; ++i) {
        tailLoss += -sortedReturns[i];
    }

    // Add the fractional observation when the tail does not
    // contain a whole number of observations.
    const double fractionalPart =
        tailSize - static_cast<double>(fullTailCount);

    if (fractionalPart > 0.0 &&
        fullTailCount < sortedReturns.size()) {
        tailLoss +=
            fractionalPart * (-sortedReturns[fullTailCount]);
    }

    double expectedShortfall = tailLoss / tailSize;

    // Risk metrics represent potential loss.
    // If the historical tail contains no losses, report zero
    // rather than a negative risk value.
    expectedShortfall =
        std::max(0.0, expectedShortfall);

    return {
        returns,
        var,
        expectedShortfall
    };
}

} // namespace ql