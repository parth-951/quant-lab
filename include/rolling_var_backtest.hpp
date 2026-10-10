#pragma once

#include "var_backtesting.hpp"

#include <cstddef>
#include <vector>

namespace ql {

struct RollingVaRBacktestResult {
    // Forecasts and realized losses, aligned by forecast day.
    std::vector<double> predictedVaR;
    std::vector<double> realizedLosses;

    // Original return-series index for each forecast.
    std::vector<std::size_t> forecastIndices;

    // Results from the existing Kupiec backtester.
    VaRBacktestResult backtest;
};

class RollingVaRBacktest {
public:
    // Returns are decimal daily portfolio returns, e.g. -0.02 = -2%.
    // The portfolio value is held constant across the backtest.
    //
    // Each forecast uses exactly the preceding lookbackWindow returns.
    // The next return is used only to calculate the realized loss.
    [[nodiscard]] static RollingVaRBacktestResult run(
        const std::vector<double>& returns,
        std::size_t lookbackWindow,
        double portfolioValue,
        double confidenceLevel,
        double significanceLevel = 0.05
    );
};

} // namespace ql