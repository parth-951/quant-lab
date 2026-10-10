
#pragma once

#include "portfolio.hpp"

#include <map>
#include <string>
#include <vector>

namespace ql {

enum class StressScenarioType {
    EquityCrash,
    RateShock,
    VolatilitySpike,
    CurrencyShock,
    Combined,
    Custom
};

struct StressScenario {
    std::string name;
    StressScenarioType type;
    // Decimal price shocks: -0.30 means -30%, 0.10 means +10%.
    // Instruments omitted from this map receive a zero price shock.
    std::map<std::string, double> priceShocks;
};

struct StressPositionResult {
    std::string instrumentId;
    double quantity;
    double currentPrice;
    double shockedPrice;
    double currentMarketValue;
    double shockedMarketValue;
    double stressPnl;
};

struct StressInstrumentResult {
    std::string instrumentId;
    double currentMarketValue;
    double shockedMarketValue;
    double stressPnl;
};

struct PortfolioStressResult {
    std::string scenarioName;
    StressScenarioType scenarioType;

    double currentNetExposure;
    double currentGrossExposure;

    double shockedNetExposure;
    double shockedGrossExposure;

    // Change in portfolio market value caused by the scenario.
    double stressPnl;

    std::vector<StressPositionResult> positions;
    std::vector<StressInstrumentResult> instruments;
};

class PortfolioStressTester {
public:
    // Returns scenarios ordered from worst stress P&L to best.
    // A more negative stressPnl means a larger loss.
    std::vector<PortfolioStressResult> calculate(
        const Portfolio& portfolio,
        const std::vector<StressScenario>& scenarios
    ) const;
};

} // namespace ql
