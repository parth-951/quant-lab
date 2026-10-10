
#include "portfolio_stress.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <vector>

using namespace ql;

namespace {

void expectNear(
    std::string_view name,
    double actual,
    double expected,
    double tolerance = 1e-10
) {
    if (std::abs(actual - expected) > tolerance) {
        std::cerr
            << "FAILED: " << name
            << "\nExpected: " << expected
            << "\nActual: " << actual
            << "\n";
        std::exit(EXIT_FAILURE);
    }
}

void expectTrue(std::string_view name, bool condition) {
    if (!condition) {
        std::cerr << "FAILED: " << name << "\n";
        std::exit(EXIT_FAILURE);
    }
}

template <typename Function>
void expectThrows(
    std::string_view name,
    Function function
) {
    try {
        function();
        std::cerr
            << "FAILED: " << name
            << "\nExpected invalid_argument.\n";
        std::exit(EXIT_FAILURE);
    } catch (const std::invalid_argument&) {
        // Expected.
    }
}

Portfolio makePortfolio() {
    Portfolio portfolio;
    portfolio.addPosition({10.0, 90.0, 100.0, "A"});
    portfolio.addPosition({-5.0, 110.0, 100.0, "B"});
    return portfolio;
}

} // namespace

int main() {
    PortfolioStressTester tester;

    // TEST 1: Long positions lose and short positions gain
    // when both prices fall by 20%.
    {
        Portfolio portfolio = makePortfolio();

        const auto results = tester.calculate(
            portfolio,
            {
                {
                    "Equity crash",
                    StressScenarioType::EquityCrash,
                    {{"A", -0.20}, {"B", -0.20}}
                }
            }
        );

        expectTrue("One scenario returned", results.size() == 1);

        const auto& result = results.front();

        expectNear("Long position stress P&L",
                   result.positions[0].stressPnl, -200.0);

        expectNear("Short position stress P&L",
                   result.positions[1].stressPnl, 100.0);

        expectNear("Portfolio stress P&L", result.stressPnl, -100.0);
        expectNear("Current net exposure",
                   result.currentNetExposure, 500.0);
        expectNear("Current gross exposure",
                   result.currentGrossExposure, 1500.0);
        expectNear("Shocked net exposure",
                   result.shockedNetExposure, 400.0);
        expectNear("Shocked gross exposure",
                   result.shockedGrossExposure, 1200.0);
    }

    // TEST 2: Combined shocks and worst-scenario ranking.
    {
        Portfolio portfolio = makePortfolio();

        const auto results = tester.calculate(
            portfolio,
            {
                {
                    "Mild shock",
                    StressScenarioType::Custom,
                    {{"A", -0.05}, {"B", -0.05}}
                },
                {
                    "Combined shock",
                    StressScenarioType::Combined,
                    {{"A", -0.30}, {"B", 0.10}}
                },
                {
                    "Rate shock",
                    StressScenarioType::RateShock,
                    {{"A", -0.10}, {"B", 0.02}}
                }
            }
        );

        expectTrue("Three scenarios returned", results.size() == 3);

        expectTrue(
            "Worst scenario ranked first",
            results[0].scenarioName == "Combined shock"
        );

        expectNear(
            "Combined shock P&L",
            results[0].stressPnl,
            -350.0
        );

        expectNear(
            "Rate shock P&L",
            results[1].stressPnl,
            -110.0
        );

        expectNear(
            "Mild shock P&L",
            results[2].stressPnl,
            -25.0
        );
    }

    // TEST 3: Missing instrument shocks are treated as zero.
    {
        Portfolio portfolio = makePortfolio();

        const auto results = tester.calculate(
            portfolio,
            {
                {
                    "Partial shock",
                    StressScenarioType::Custom,
                    {{"A", -0.10}}
                }
            }
        );

        const auto& result = results.front();

        expectNear("Shocked A price",
                   result.positions[0].shockedPrice, 90.0);

        expectNear("Unshocked B price",
                   result.positions[1].shockedPrice, 100.0);

        expectNear("Partial shock P&L", result.stressPnl, -100.0);
    }

    // TEST 4: Multiple lots aggregate into one instrument result.
    {
        Portfolio portfolio;
        portfolio.addPosition({10.0, 90.0, 100.0, "A"});
        portfolio.addPosition({5.0, 95.0, 100.0, "A"});
        portfolio.addPosition({-5.0, 110.0, 100.0, "B"});

        const auto results = tester.calculate(
            portfolio,
            {
                {
                    "Crash",
                    StressScenarioType::EquityCrash,
                    {{"A", -0.10}, {"B", -0.10}}
                }
            }
        );

        const auto& result = results.front();

        expectTrue(
            "Three position-level results",
            result.positions.size() == 3
        );

        expectTrue(
            "Two aggregated instrument results",
            result.instruments.size() == 2
        );

        expectTrue(
            "Instrument A appears first",
            result.instruments[0].instrumentId == "A"
        );

        expectNear(
            "Aggregated A stress P&L",
            result.instruments[0].stressPnl,
            -150.0
        );

        expectNear(
            "Aggregated B stress P&L",
            result.instruments[1].stressPnl,
            50.0
        );

        expectNear("Total stress P&L", result.stressPnl, -100.0);
    }

    // TEST 5: Invalid shocks are rejected.
    {
        Portfolio portfolio = makePortfolio();

        expectThrows("A -100% shock is rejected", [&]() {
            tester.calculate(
                portfolio,
                {
                    {
                        "Invalid",
                        StressScenarioType::Custom,
                        {{"A", -1.0}}
                    }
                }
            );
        });

        expectThrows("Unknown instrument is rejected", [&]() {
            tester.calculate(
                portfolio,
                {
                    {
                        "Unknown",
                        StressScenarioType::Custom,
                        {{"NOT_HELD", -0.10}}
                    }
                }
            );
        });

        expectThrows("Duplicate scenario names are rejected", [&]() {
            tester.calculate(
                portfolio,
                {
                    {"Same", StressScenarioType::Custom, {}},
                    {"Same", StressScenarioType::Combined, {}}
                }
            );
        });

        expectThrows("Empty scenario list is rejected", [&]() {
            tester.calculate(portfolio, {});
        });

        Portfolio emptyPortfolio;

        expectThrows("Empty portfolio is rejected", [&]() {
            tester.calculate(
                emptyPortfolio,
                {
                    {"Crash", StressScenarioType::EquityCrash, {}}
                }
            );
        });
    }

    std::cout << "All Portfolio Stress tests passed.\n";
    return 0;
}
