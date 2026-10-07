#include "risk_attribution.hpp"

#include <algorithm>
#include <cmath>
#include <map>
#include <stdexcept>
#include <vector>

namespace ql {

RiskAttributionResult RiskAttribution::calculate(
    const PortfolioHistoricalRiskResult& riskResult,
    double confidenceLevel
) const {

    if (!std::isfinite(confidenceLevel) ||
        confidenceLevel <= 0.0 ||
        confidenceLevel >= 1.0) {

        throw std::invalid_argument(
            "Confidence level must be between 0 and 1"
        );
    }

    const std::size_t scenarioCount =
        riskResult.scenarioPnl.size();

    if (scenarioCount == 0) {
        throw std::invalid_argument(
            "Risk result must contain scenario P&Ls"
        );
    }

    if (riskResult.scenarioDates.size() != scenarioCount) {
        throw std::invalid_argument(
            "Scenario dates and scenario P&Ls must have the same size"
        );
    }

    // ------------------------------------------------------------
    // Validate per-instrument scenario P&Ls.
    // ------------------------------------------------------------

    for (const auto& [instrumentId, instrumentPnl] :
         riskResult.instrumentScenarioPnl) {

        if (instrumentPnl.size() != scenarioCount) {
            throw std::invalid_argument(
                "Instrument scenario P&Ls must align with "
                "portfolio scenarios"
            );
        }
    }

    // ------------------------------------------------------------
    // Sort scenario indices by total scenario P&L:
    //
    // worst loss -> best outcome
    //
    // We sort indices rather than the P&Ls themselves so that
    // instrument P&Ls remain aligned with their scenarios.
    // ------------------------------------------------------------

    std::vector<std::size_t> sortedIndices(scenarioCount);

    for (std::size_t i = 0; i < scenarioCount; ++i) {
        sortedIndices[i] = i;
    }

    std::sort(
        sortedIndices.begin(),
        sortedIndices.end(),
        [&](std::size_t lhs, std::size_t rhs) {
            return riskResult.scenarioPnl[lhs] <
                   riskResult.scenarioPnl[rhs];
        }
    );

    // ------------------------------------------------------------
    // Same Expected Shortfall tail definition used by
    // HistoricalRisk.
    //
    // tailSize = (1 - confidence) * number of observations
    //
    // Take:
    //   floor(tailSize) full observations
    //   + fractional part of the next observation
    // ------------------------------------------------------------

    const double tailSize =
        (1.0 - confidenceLevel) *
        static_cast<double>(scenarioCount);

    if (tailSize <= 0.0) {
        throw std::invalid_argument(
            "Expected Shortfall tail must contain observations"
        );
    }

    const std::size_t fullTailCount =
        static_cast<std::size_t>(
            std::floor(tailSize)
        );

    const double fractionalPart =
        tailSize -
        static_cast<double>(fullTailCount);

    // ------------------------------------------------------------
    // Calculate component ES.
    // ------------------------------------------------------------

    std::map<std::string, double> componentTailLoss;

    for (const auto& [instrumentId, instrumentPnl] :
         riskResult.instrumentScenarioPnl) {

        double tailLoss = 0.0;

        // Full tail observations.
        for (std::size_t i = 0; i < fullTailCount; ++i) {

            const std::size_t scenarioIndex =
                sortedIndices[i];

            tailLoss += -instrumentPnl[scenarioIndex];
        }

        // Fractional tail observation.
        if (fractionalPart > 0.0 &&
            fullTailCount < scenarioCount) {

            const std::size_t scenarioIndex =
                sortedIndices[fullTailCount];

            tailLoss +=
                fractionalPart *
                (-instrumentPnl[scenarioIndex]);
        }

        // Do not clamp component ES.
        // A negative value represents a hedge.
        componentTailLoss[instrumentId] =
            tailLoss / tailSize;
    }

    // ------------------------------------------------------------
    // Portfolio Expected Shortfall.
    // ------------------------------------------------------------

    const double totalES =
        riskResult.expectedShortfall;

    // ------------------------------------------------------------
    // Reconcile component ES against portfolio ES.
    //
    // The sum of all component ES values must equal the
    // portfolio Expected Shortfall within numerical tolerance.
    // ------------------------------------------------------------

    double componentESTotal = 0.0;

    for (const auto& [instrumentId, componentES] :
         componentTailLoss) {

        componentESTotal += componentES;
    }

    const double reconciliationTolerance =
        1e-10 * std::max(1.0, std::abs(totalES));

    if (std::abs(componentESTotal - totalES) >
        reconciliationTolerance) {

        throw std::runtime_error(
            "Risk attribution reconciliation failed: "
            "sum of component ES does not equal portfolio ES"
        );
    }

    // ------------------------------------------------------------
    // Calculate percentage contribution.
    // ------------------------------------------------------------

    std::map<std::string, double> contributionPercent;

    if (std::abs(totalES) > 1e-15) {

        for (const auto& [instrumentId, componentES] :
             componentTailLoss) {

            contributionPercent[instrumentId] =
                (componentES / totalES) * 100.0;
        }

    } else {

        // If portfolio ES is zero, percentage contribution is
        // undefined. Return zero rather than dividing by zero.

        for (const auto& [instrumentId, componentES] :
             componentTailLoss) {

            contributionPercent[instrumentId] = 0.0;
        }
    }

    // ------------------------------------------------------------
    // Return complete attribution result.
    // ------------------------------------------------------------

    return {
        componentTailLoss,
        contributionPercent,
        totalES
    };
}

} // namespace ql