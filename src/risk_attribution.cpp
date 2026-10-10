#include "risk_attribution.hpp"

#include <algorithm>
#include <cmath>
#include <map>
#include <stdexcept>
#include <string>
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

    if (riskResult.instrumentScenarioPnl.empty()) {
        throw std::invalid_argument(
            "Risk result must contain instrument scenario P&Ls"
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
    // Validate that instrument P&Ls sum to the portfolio P&L
    // for every scenario, not just the ES-tail scenarios.
    // ------------------------------------------------------------

    for (std::size_t scenarioIndex = 0;
         scenarioIndex < scenarioCount;
         ++scenarioIndex) {

        double instrumentPnlTotal = 0.0;

        for (const auto& [instrumentId, instrumentPnl] :
             riskResult.instrumentScenarioPnl) {

            instrumentPnlTotal += instrumentPnl[scenarioIndex];
        }

        const double portfolioPnl =
            riskResult.scenarioPnl[scenarioIndex];

        const double tolerance =
            1e-10 * std::max({
                1.0,
                std::abs(instrumentPnlTotal),
                std::abs(portfolioPnl)
            });

        if (!std::isfinite(instrumentPnlTotal) ||
            !std::isfinite(portfolioPnl) ||
            std::abs(instrumentPnlTotal - portfolioPnl) > tolerance) {

            throw std::invalid_argument(
                "Instrument scenario P&Ls do not sum to portfolio "
                "scenario P&L at scenario index " +
                std::to_string(scenarioIndex)
            );
        }
    }

    // ------------------------------------------------------------
    // Portfolio ES follows the portfolio risk engine convention:
    // ES is clamped to zero when the raw tail loss is negative.
    // In that case, component ES values must also be zero so
    // that attribution reconciles with the reported portfolio ES.
    // ------------------------------------------------------------

    const double totalES =
        riskResult.expectedShortfall;

    if (!std::isfinite(totalES) || totalES < 0.0) {
        throw std::invalid_argument(
            "Portfolio Expected Shortfall must be finite and non-negative"
        );
    }

    if (totalES == 0.0) {
        std::map<std::string, double> componentES;
        std::map<std::string, double> contributionPercent;

        for (const auto& [instrumentId, instrumentPnl] :
             riskResult.instrumentScenarioPnl) {
            componentES[instrumentId] = 0.0;
            contributionPercent[instrumentId] = 0.0;
        }

        return {
            componentES,
            contributionPercent,
            totalES
        };
    }

    // ------------------------------------------------------------
    // Sort scenario indices by total scenario P&L:
    // worst loss -> best outcome.
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
    // Use the same Expected Shortfall tail definition as the
    // historical risk engine, including fractional observations.
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
        static_cast<std::size_t>(std::floor(tailSize));

    const double fractionalPart =
        tailSize - static_cast<double>(fullTailCount);

    // ------------------------------------------------------------
    // Calculate signed component ES.
    // Negative component ES is retained for hedging positions
    // when total portfolio ES is positive.
    // ------------------------------------------------------------

    std::map<std::string, double> componentES;

    for (const auto& [instrumentId, instrumentPnl] :
         riskResult.instrumentScenarioPnl) {

        double tailLoss = 0.0;

        for (std::size_t i = 0; i < fullTailCount; ++i) {
            const std::size_t scenarioIndex = sortedIndices[i];
            tailLoss += -instrumentPnl[scenarioIndex];
        }

        if (fractionalPart > 0.0 &&
            fullTailCount < scenarioCount) {

            const std::size_t scenarioIndex =
                sortedIndices[fullTailCount];

            tailLoss +=
                fractionalPart * (-instrumentPnl[scenarioIndex]);
        }

        componentES[instrumentId] = tailLoss / tailSize;
    }

    // ------------------------------------------------------------
    // Reconcile component ES against the reported portfolio ES.
    // ------------------------------------------------------------

    double componentESTotal = 0.0;

    for (const auto& [instrumentId, value] : componentES) {
        componentESTotal += value;
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
    // Calculate percentage contributions.
    // ------------------------------------------------------------

    std::map<std::string, double> contributionPercent;

    for (const auto& [instrumentId, value] : componentES) {
        contributionPercent[instrumentId] =
            (value / totalES) * 100.0;
    }

    return {
        componentES,
        contributionPercent,
        totalES
    };
}

} // namespace ql
