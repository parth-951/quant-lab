#pragma once

#include <string>
#include <vector>
#include <map>

#include "portfolio.hpp"

namespace ql {

/*
 * Historical dates must use ISO 8601 calendar format:
 *
 *     YYYY-MM-DD
 *
 * Example:
 *
 *     2026-01-31
 *
 * This format is required because dates are stored as strings and
 * lexicographical ordering is used to preserve chronological order.
 */
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

    // Per-instrument scenario P&L.
    // Key: instrument ID
    // Value: scenario P&L aligned with scenarioDates.
    std::map<std::string, std::vector<double>> instrumentScenarioPnl;

    double var;
    double expectedShortfall;
};

class PortfolioHistoricalRisk {
public:
    PortfolioHistoricalRiskResult calculate(
        const Portfolio& portfolio,
        const std::vector<HistoricalPriceSeries>& histories,
        double confidenceLevel
    ) const;
};

} // namespace ql