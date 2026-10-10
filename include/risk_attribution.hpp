#pragma once

#include "portfolio_historical_risk.hpp"

#include <map>
#include <string>

namespace ql {

struct RiskAttributionResult {
    // Positive = contributes to portfolio tail loss.
    // Negative = offsets portfolio tail loss.
    std::map<std::string, double> componentES;

    // Component ES contribution expressed in percentage points.
    // Example: 25.0 = 25%.
    std::map<std::string, double> contributionPercent;

    // Portfolio Expected Shortfall.
    double totalES;
};

class RiskAttribution {
public:
    RiskAttributionResult calculate(
        const PortfolioHistoricalRiskResult& riskResult,
        double confidenceLevel
    ) const;
};

} // namespace ql