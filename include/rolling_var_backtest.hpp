
#pragma once

#include "var_backtesting.hpp"

#include <cstddef>
#include <vector>

namespace ql {

enum class PortfolioValueMode {
    FixedNotional,
    Compounded
};

struct RollingVaRBacktestResult {
    std::vector<double> predictedVaR;
    std::vector<double> realizedLosses;
    std::vector<std::size_t> forecastIndices;
    std::vector<double> forecastPortfolioValues;
    VaRBacktestResult backtest;
};

class RollingVaRBacktest {
public:
    [[nodiscard]] static RollingVaRBacktestResult run(
        const std::vector<double>& returns,
        std::size_t lookbackWindow,
        double portfolioValue,
        double confidenceLevel,
        double significanceLevel = 0.05,
        PortfolioValueMode mode = PortfolioValueMode::FixedNotional);

    [[nodiscard]] static RollingVaRBacktestResult runWithRealizedPnl(
        const std::vector<double>& returns,
        const std::vector<double>& realizedPnl,
        std::size_t lookbackWindow,
        double initialPortfolioValue,
        double confidenceLevel,
        double significanceLevel = 0.05,
        PortfolioValueMode mode = PortfolioValueMode::FixedNotional);

    [[nodiscard]] static RollingVaRBacktestResult runPnl(
        const std::vector<double>& predictedVaR,
        const std::vector<double>& realizedPnl,
        double confidenceLevel,
        double significanceLevel = 0.05);

    [[nodiscard]] static RollingVaRBacktestResult runPortfolioPnl(
        const std::vector<double>& predictedVaR,
        const std::vector<double>& realizedPnl,
        const std::vector<double>& portfolioValues,
        double confidenceLevel,
        double significanceLevel = 0.05);
};

} // namespace ql
