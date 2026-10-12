
#include "rolling_var_backtest.hpp"

#include "historical_var.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <vector>

namespace ql {
namespace {

void validateBacktestLevels(
    double confidenceLevel,
    double significanceLevel) {

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
}

void validatePortfolioValue(double portfolioValue) {
    if (!std::isfinite(portfolioValue) || portfolioValue < 0.0) {
        throw std::invalid_argument(
            "Portfolio value must be finite and nonnegative.");
    }
}

} // namespace

RollingVaRBacktestResult RollingVaRBacktest::run(
    const std::vector<double>& returns,
    std::size_t lookbackWindow,
    double portfolioValue,
    double confidenceLevel,
    double significanceLevel,
    PortfolioValueMode valueMode) {

    if (lookbackWindow == 0) {
        throw std::invalid_argument(
            "Lookback window must be at least one observation.");
    }

    if (returns.size() <= lookbackWindow) {
        throw std::invalid_argument(
            "Returns must contain more observations than the lookback window.");
    }

    validatePortfolioValue(portfolioValue);
    validateBacktestLevels(confidenceLevel, significanceLevel);

    if (valueMode != PortfolioValueMode::FixedNotional &&
        valueMode != PortfolioValueMode::Compounded) {
        throw std::invalid_argument("Invalid portfolio value mode.");
    }

    for (double r : returns) {
        const bool invalidReturn =
            !std::isfinite(r) ||
            (valueMode == PortfolioValueMode::FixedNotional
                ? r <= -1.0
                : r < -1.0);

        if (invalidReturn) {
            throw std::invalid_argument(
                valueMode == PortfolioValueMode::FixedNotional
                    ? "Fixed-notional returns must be finite and greater than -100%."
                    : "Compounded returns must be finite and at least -100%.");
        }
    }

    RollingVaRBacktestResult result;

    const std::size_t forecastCount =
        returns.size() - lookbackWindow;

    result.predictedVaR.reserve(forecastCount);
    result.realizedLosses.reserve(forecastCount);
    result.forecastIndices.reserve(forecastCount);

    HistoricalVaR historicalVaR;

    // Value at the start of the next forecast day.
    double currentPortfolioValue = portfolioValue;

if (valueMode == PortfolioValueMode::Compounded) {
    // Returns before the first forecast have already affected the
    // portfolio's starting value for that forecast.
    for (std::size_t i = 0; i < lookbackWindow; ++i) {
        currentPortfolioValue *= (1.0 + returns[i]);

        if (!std::isfinite(currentPortfolioValue)) {
            throw std::overflow_error(
                "Compounded portfolio value is not finite.");
        }
    }
}

    for (std::size_t t = lookbackWindow;
         t < returns.size();
         ++t) {

        // Strictly historical window [t - lookbackWindow, t).
        // The return at t cannot affect this forecast.
        const std::vector<double> window(
            returns.begin() +
                static_cast<std::ptrdiff_t>(t - lookbackWindow),
            returns.begin() + static_cast<std::ptrdiff_t>(t));

        const double historicalVaRFraction =
            historicalVaR.calculate(window, confidenceLevel);

        const double forecastPortfolioValue =
            valueMode == PortfolioValueMode::FixedNotional
                ? portfolioValue
                : currentPortfolioValue;

        const double predictedLossThreshold =
            forecastPortfolioValue * historicalVaRFraction;

        // Realized loss uses the same start-of-day notional as the forecast.
        const double realizedLoss =
            std::max(0.0, -forecastPortfolioValue * returns[t]);

        if (!std::isfinite(predictedLossThreshold) ||
            !std::isfinite(realizedLoss)) {
            throw std::overflow_error(
                "Rolling VaR calculation produced a non-finite value.");
        }

        result.predictedVaR.push_back(predictedLossThreshold);
        result.realizedLosses.push_back(realizedLoss);
        result.forecastIndices.push_back(t);

        // Compound only after processing the current day's forecast and P&L.
        // This means returns[t] cannot influence the forecast for day t.
        if (valueMode == PortfolioValueMode::Compounded) {
            currentPortfolioValue =
                forecastPortfolioValue * (1.0 + returns[t]);

            if (!std::isfinite(currentPortfolioValue)) {
                throw std::overflow_error(
                    "Compounded portfolio value is not finite.");
            }
        }
    }

    result.backtest = VaRBacktesting::run(
        result.predictedVaR,
        result.realizedLosses,
        confidenceLevel,
        significanceLevel);

    return result;
}

RollingVaRBacktestResult RollingVaRBacktest::runPnl(
    const std::vector<double>& predictedVaR,
    const std::vector<double>& realizedPnl,
    double confidenceLevel,
    double significanceLevel) {

    validateBacktestLevels(confidenceLevel, significanceLevel);

    if (predictedVaR.empty()) {
        throw std::invalid_argument(
            "P&L backtest requires at least one observation.");
    }

    if (predictedVaR.size() != realizedPnl.size()) {
        throw std::invalid_argument(
            "Predicted VaR and realized P&L must have equal lengths.");
    }

    RollingVaRBacktestResult result;

    result.predictedVaR.reserve(predictedVaR.size());
    result.realizedLosses.reserve(realizedPnl.size());
    result.forecastIndices.reserve(realizedPnl.size());

    for (std::size_t i = 0; i < predictedVaR.size(); ++i) {
        if (!std::isfinite(predictedVaR[i]) ||
            predictedVaR[i] < 0.0) {
            throw std::invalid_argument(
                "Predicted VaR must be finite and nonnegative.");
        }

        if (!std::isfinite(realizedPnl[i])) {
            throw std::invalid_argument(
                "Realized P&L must contain only finite values.");
        }

        result.predictedVaR.push_back(predictedVaR[i]);
        result.realizedLosses.push_back(
            std::max(0.0, -realizedPnl[i]));
        result.forecastIndices.push_back(i);
    }

    result.backtest = VaRBacktesting::run(
        result.predictedVaR,
        result.realizedLosses,
        confidenceLevel,
        significanceLevel);

    return result;
}
RollingVaRBacktestResult RollingVaRBacktest::runPortfolioPnl(
    const std::vector<double>& predictedVaR,
    const std::vector<double>& realizedPnl,
    const std::vector<double>& portfolioValues,
    double confidenceLevel,
    double significanceLevel) {

    validateBacktestLevels(confidenceLevel, significanceLevel);

    if (predictedVaR.empty()) {
        throw std::invalid_argument(
            "Portfolio P&L backtest requires at least one observation.");
    }

    if (predictedVaR.size() != realizedPnl.size() ||
        predictedVaR.size() != portfolioValues.size()) {
        throw std::invalid_argument(
            "VaR, realized P&L, and portfolio values must have equal lengths.");
    }

    RollingVaRBacktestResult result;

    result.predictedVaR.reserve(predictedVaR.size());
    result.realizedLosses.reserve(realizedPnl.size());
    result.forecastIndices.reserve(realizedPnl.size());

    for (std::size_t i = 0; i < predictedVaR.size(); ++i) {
        if (!std::isfinite(predictedVaR[i]) ||
            predictedVaR[i] < 0.0) {
            throw std::invalid_argument(
                "Predicted VaR must be finite and nonnegative.");
        }

        if (!std::isfinite(realizedPnl[i])) {
            throw std::invalid_argument(
                "Realized P&L must contain only finite values.");
        }

        if (!std::isfinite(portfolioValues[i]) ||
            portfolioValues[i] < 0.0) {
            throw std::invalid_argument(
                "Portfolio values must be finite and nonnegative.");
        }

        result.predictedVaR.push_back(predictedVaR[i]);
        result.realizedLosses.push_back(
            std::max(0.0, -realizedPnl[i]));
        result.forecastIndices.push_back(i);
    }

    result.backtest = VaRBacktesting::run(
        result.predictedVaR,
        result.realizedLosses,
        confidenceLevel,
        significanceLevel);

    return result;
}
} // namespace ql
