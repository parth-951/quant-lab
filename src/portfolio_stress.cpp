
#include "portfolio_stress.hpp"

#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <stdexcept>
#include <utility>

namespace ql {

std::vector<PortfolioStressResult> PortfolioStressTester::calculate(
    const Portfolio& portfolio,
    const std::vector<StressScenario>& scenarios
) const {
    if (portfolio.positions().empty()) {
        throw std::invalid_argument(
            "Portfolio must contain at least one position"
        );
    }

    if (scenarios.empty()) {
        throw std::invalid_argument(
            "At least one stress scenario is required"
        );
    }

    std::set<std::string> portfolioInstruments;

    for (const Position& position : portfolio.positions()) {
        portfolioInstruments.insert(position.instrumentId);
    }

    std::set<std::string> scenarioNames;

    std::vector<PortfolioStressResult> results;
    results.reserve(scenarios.size());

    for (const StressScenario& scenario : scenarios) {
        if (scenario.name.empty()) {
            throw std::invalid_argument(
                "Stress scenario name must not be empty"
            );
        }

        if (!scenarioNames.insert(scenario.name).second) {
            throw std::invalid_argument(
                "Stress scenario names must be unique"
            );
        }

        for (const auto& [instrumentId, shock] :
             scenario.priceShocks) {
            if (instrumentId.empty()) {
                throw std::invalid_argument(
                    "Stress shock instrument ID must not be empty"
                );
            }

            if (portfolioInstruments.find(instrumentId) ==portfolioInstruments.end()) { {
                throw std::invalid_argument(
                    "Stress scenario references an instrument "
                    "not present in the portfolio: " + instrumentId
                );
            }
        }

            if (!std::isfinite(shock) || shock <= -1.0) {
                throw std::invalid_argument(
                    "Price shocks must be finite and greater than -100%"
                );
            }
        }

        PortfolioStressResult result{
            scenario.name,
            scenario.type,
            0.0,
            0.0,
            0.0,
            0.0,
            0.0,
            {},
            {}
        };

        result.positions.reserve(portfolio.positions().size());

        // Aggregate multiple lots of the same instrument.
        std::map<std::string, StressInstrumentResult> instruments;

        for (const Position& position : portfolio.positions()) {
            double shock = 0.0;

            const auto shockIt =
                scenario.priceShocks.find(position.instrumentId);

            if (shockIt != scenario.priceShocks.end()) {
                shock = shockIt->second;
            }

            const double currentPrice = position.currentPrice;
            const double shockedPrice = currentPrice * (1.0 + shock);

            const double currentMarketValue =
                position.quantity * currentPrice;

            const double shockedMarketValue =
                position.quantity * shockedPrice;

            const double stressPnl =
                shockedMarketValue - currentMarketValue;

            if (!std::isfinite(shockedPrice) ||
                shockedPrice <= 0.0 ||
                !std::isfinite(currentMarketValue) ||
                !std::isfinite(shockedMarketValue) ||
                !std::isfinite(stressPnl)) {
                throw std::invalid_argument(
                    "Stress calculation produced a non-finite "
                    "or non-positive value for instrument: " +
                    position.instrumentId
                );
            }

            result.positions.push_back({
                position.instrumentId,
                position.quantity,
                currentPrice,
                shockedPrice,
                currentMarketValue,
                shockedMarketValue,
                stressPnl
            });

            result.currentNetExposure += currentMarketValue;
            result.currentGrossExposure +=
                std::abs(currentMarketValue);

            result.shockedNetExposure += shockedMarketValue;
            result.shockedGrossExposure +=
                std::abs(shockedMarketValue);

            result.stressPnl += stressPnl;

            auto [it, inserted] = instruments.try_emplace(
                position.instrumentId,
                StressInstrumentResult{
                    position.instrumentId,
                    0.0,
                    0.0,
                    0.0
                }
            );

            StressInstrumentResult& instrument = it->second;

            instrument.currentMarketValue += currentMarketValue;
            instrument.shockedMarketValue += shockedMarketValue;
            instrument.stressPnl += stressPnl;
        }

        result.instruments.reserve(instruments.size());

        for (const auto& [instrumentId, instrument] : instruments) {
            (void)instrumentId;
            result.instruments.push_back(instrument);
        }

        if (!std::isfinite(result.currentNetExposure) ||
            !std::isfinite(result.currentGrossExposure) ||
            !std::isfinite(result.shockedNetExposure) ||
            !std::isfinite(result.shockedGrossExposure) ||
            !std::isfinite(result.stressPnl)) {
            throw std::invalid_argument(
                "Stress scenario produced non-finite portfolio totals"
            );
        }

        results.push_back(std::move(result));
    }

    // Worst scenario first: the most negative stress P&L comes first.
    std::stable_sort(
        results.begin(),
        results.end(),
        [](const PortfolioStressResult& left,
           const PortfolioStressResult& right) {
            return left.stressPnl < right.stressPnl;
        }
    );

    return results;
}

} // namespace ql
