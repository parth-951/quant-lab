#include "risk_comparison.hpp"

#include "historical_risk.hpp"
#include "parametric_var.hpp"
#include "parametric_es.hpp"

#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <vector>

namespace ql {

RiskComparisonResult RiskComparison::calculate(
    const std::vector<double>& returns,
    double portfolioValue,
    const std::vector<double>& weights,
    const CovarianceMatrix::Matrix& covarianceMatrix,
    double confidenceLevel,
    std::size_t horizonDays) {

    if (returns.empty()) {
        throw std::invalid_argument(
            "Portfolio returns must not be empty."
        );
    }

    if (!std::isfinite(portfolioValue) || portfolioValue < 0.0) {
        throw std::invalid_argument(
            "Portfolio value must be finite and nonnegative."
        );
    }

    if (!std::isfinite(confidenceLevel) ||
        confidenceLevel <= 0.5 ||
        confidenceLevel >= 1.0) {
        throw std::invalid_argument(
            "Confidence level must be greater than 0.5 and less than 1.0."
        );
    }

    if (horizonDays == 0 || horizonDays > returns.size()) {
        throw std::invalid_argument(
            "Horizon must be positive and no longer than the return history."
        );
    }

    for (double r : returns) {
        if (!std::isfinite(r) || r <= -1.0) {
            throw std::invalid_argument(
                "Returns must be finite and greater than -100%."
            );
        }
    }

    // Build rolling compounded returns over the requested horizon.
    std::vector<double> horizonReturns;
    horizonReturns.reserve(returns.size() - horizonDays + 1);

    for (std::size_t end = horizonDays - 1;
         end < returns.size(); ++end) {
        double growth = 1.0;

        for (std::size_t k = end - horizonDays + 1;
             k <= end; ++k) {
            growth *= 1.0 + returns[k];

            if (!std::isfinite(growth)) {
                throw std::overflow_error(
                    "Compounded historical return is not finite."
                );
            }
        }

        const double compoundedReturn = growth - 1.0;

        if (!std::isfinite(compoundedReturn)) {
            throw std::overflow_error(
                "Compounded historical return is not finite."
            );
        }

        horizonReturns.push_back(compoundedReturn);
    }

    // Reuse the shared HistoricalRisk implementation for both metrics.
    HistoricalRisk historicalRisk;

    const HistoricalRiskResult historicalResult =
        historicalRisk.calculateFromReturns(
            horizonReturns,
            confidenceLevel
        );

    const double historicalVaR =
        portfolioValue * historicalResult.var;

    const double historicalES =
        portfolioValue * historicalResult.expectedShortfall;

    if (!std::isfinite(historicalVaR) ||
        !std::isfinite(historicalES)) {
        throw std::overflow_error(
            "Historical risk calculation produced a non-finite result."
        );
    }

    // Parametric engines retain their existing APIs.
    const double parametricVaR = ParametricVaR::calculate(
        portfolioValue,
        weights,
        covarianceMatrix,
        confidenceLevel,
        horizonDays
    );

    const double parametricES = ParametricES::calculate(
        portfolioValue,
        weights,
        covarianceMatrix,
        confidenceLevel,
        horizonDays
    );

    return {
        historicalVaR,
        historicalES,
        parametricVaR,
        parametricES,
        portfolioValue,
        confidenceLevel,
        horizonDays
    };
}

} // namespace ql