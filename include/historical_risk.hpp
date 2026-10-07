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
    HistoricalRiskResult calculate(
        const std::vector<double>& prices,
        double confidenceLevel
    ) const;
};

} // namespace ql