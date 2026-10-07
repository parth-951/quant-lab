#pragma once

#include "portfolio.hpp"

#include <string>
#include <vector>

namespace ql {

struct InstrumentExposure {
    std::string instrumentId;
    double marketValue;
    double grossWeight;
};

struct PortfolioExposureResult {
    double netExposure;
    double grossExposure;
    std::vector<InstrumentExposure> instruments;
};

class PortfolioExposure {
public:
    PortfolioExposureResult calculate(
        const Portfolio& portfolio
    ) const;
};

} // namespace ql