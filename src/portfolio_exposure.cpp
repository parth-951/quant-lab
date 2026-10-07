#include "portfolio_exposure.hpp"

#include <cmath>
#include <map>
#include <stdexcept>

namespace ql {

PortfolioExposureResult PortfolioExposure::calculate(
    const Portfolio& portfolio
) const {
    if (portfolio.positions().empty()) {
        throw std::invalid_argument(
            "Portfolio must contain at least one position"
        );
    }

    std::map<std::string, double> marketValues;

    double netExposure = 0.0;
    double grossExposure = 0.0;

    for (const Position& position : portfolio.positions()) {
        const double marketValue = position.marketValue();

        if (!std::isfinite(marketValue)) {
            throw std::invalid_argument(
                "Position market value must be finite"
            );
        }

        netExposure += marketValue;
        grossExposure += std::abs(marketValue);

        // Multiple lots of the same instrument are aggregated.
        marketValues[position.instrumentId] += marketValue;
    }

    std::vector<InstrumentExposure> instruments;
    instruments.reserve(marketValues.size());

    for (const auto& [instrumentId, marketValue] : marketValues) {
        const double grossWeight =
            grossExposure > 0.0
                ? std::abs(marketValue) / grossExposure
                : 0.0;

        instruments.push_back({
            instrumentId,
            marketValue,
            grossWeight
        });
    }

    return {
        netExposure,
        grossExposure,
        instruments
    };
}

} // namespace ql