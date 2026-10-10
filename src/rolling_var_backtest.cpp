#include "rolling_var_backtest.hpp"

#include "historical_var.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>

namespace ql {

RollingVaRBacktestResult RollingVaRBacktest::run(
    const std::vector<double>& returns,
    std::size_t lookbackWindow,
    double portfolioValue,
    double confidenceLevel,
    double significanceLevel) {

    if (lookbackWindow == 0) {
        throw std::invalid_argument(
            "Lookback window must be at least one observation.");
    }

    if (returns.size() <= lookbackWindow) {
        throw std::invalid_argument(
            "Returns must contain more observations than the lookback window.");
    }

    if (!std::isfinite(portfolioValue) || portfolioValue < 0.0) {
        throw std::invalid_argument(
            "Portfolio value must be finite and nonnegative.");
    }

    if (!std::isfinite(confidenceLevel) ||
        confidenceLevel <= 0.5 ||
        confidenceLevel >= 1.0) {
        throw std::invalid_argument(
            "Confidence level must be greater than 0.5 and less than 1.0.");
    }

    if (!std::isfinite(significanceLevel) ||
        significanceLevel <= 0.0 ||
        significanceLevel >= 1.0) {
        throw std::invalid_argument(
            "Significance level must be greater than 0 and less than 1.");
    }

    for (double r : returns) {
        if (!std::isfinite(r) || r <= -1.0) {
            throw std::invalid_argument(
                "Returns must be finite and greater than -100%.");
        }
    }

    RollingVaRBacktestResult result;

    const std::size_t forecastCount =
        returns.size() - lookbackWindow;

    result.predictedVaR.reserve(forecastCount);
    result.realizedLosses.reserve(forecastCount);
    result.forecastIndices.reserve(forecastCount);

    HistoricalVaR historicalVaR;

    for (std::size_t t = lookbackWindow;
         t < returns.size();
         ++t) {

        // Use only observations [t - lookbackWindow, t).
        // Return at index t is not included in the VaR estimate.
        const std::vector<double> window(
            returns.begin() +
                static_cast<std::ptrdiff_t>(t - lookbackWindow),
            returns.begin() + static_cast<std::ptrdiff_t>(t)
        );

        const double historicalVaRFraction =
            historicalVaR.calculate(window, confidenceLevel);

        const double predictedLossThreshold =
            portfolioValue * historicalVaRFraction;

        // Positive returns produce zero realized loss.
        const double realizedLoss =
            std::max(0.0, -portfolioValue * returns[t]);

        if (!std::isfinite(predictedLossThreshold) ||
            !std::isfinite(realizedLoss)) {
            throw std::overflow_error(
                "Rolling VaR calculation produced a non-finite value.");
        }

        result.predictedVaR.push_back(predictedLossThreshold);
        result.realizedLosses.push_back(realizedLoss);
        result.forecastIndices.push_back(t);
    }

    result.backtest = VaRBacktesting::run(
        result.predictedVaR,
        result.realizedLosses,
        confidenceLevel,
        significanceLevel
    );

    return result;
}

} // namespace ql