#pragma once

#include <vector>

namespace ql {

struct HistoricalRiskResult {
    std::vector<double> returns;
    double var;
    double expectedShortfall;
};

class HistoricalRisk {
public:
    // Accept prices and calculate returns internally.
    HistoricalRiskResult calculate(
        const std::vector<double>& prices,
        double confidenceLevel
    ) const;

    // Calculate historical risk directly from an existing return series.
    HistoricalRiskResult calculateFromReturns(
        const std::vector<double>& returns,
        double confidenceLevel
    ) const;
};

} // namespace ql