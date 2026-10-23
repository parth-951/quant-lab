
#pragma once

#include "historical_var.hpp"

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
        double confidenceLevel,
        HistoricalQuantileConvention convention =
            HistoricalQuantileConvention::LinearInterpolation
    ) const;

    HistoricalRiskResult calculateFromReturns(
        const std::vector<double>& returns,
        double confidenceLevel,
        HistoricalQuantileConvention convention =
            HistoricalQuantileConvention::LinearInterpolation
    ) const;
};

} // namespace ql
