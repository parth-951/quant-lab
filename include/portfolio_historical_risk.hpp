
#pragma once

#include <string>
#include <vector>
#include <map>

#include "historical_var.hpp"
#include "portfolio.hpp"

namespace ql {

struct HistoricalPriceSeries {
    std::string instrumentId;
    std::vector<std::string> dates;
    std::vector<double> prices;
};

struct PortfolioHistoricalRiskResult {
    double currentMarketValue;
    double currentPnl;
    std::vector<std::string> scenarioDates;
    std::vector<double> scenarioPnl;

    std::map<std::string, std::vector<double>> instrumentScenarioPnl;

    double var;
    double expectedShortfall;
};

class PortfolioHistoricalRisk {
public:
    PortfolioHistoricalRiskResult calculate(
        const Portfolio& portfolio,
        const std::vector<HistoricalPriceSeries>& histories,
        double confidenceLevel,
        HistoricalQuantileConvention convention =
            HistoricalQuantileConvention::LinearInterpolation
    ) const;
};

} // namespace ql
