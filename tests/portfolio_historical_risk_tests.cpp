#include "portfolio_historical_risk.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <vector>

using namespace ql;

namespace {

void expectNear(
    double actual,
    double expected,
    double tolerance,
    std::string_view message
) {
    if (std::abs(actual - expected) > tolerance) {
        std::cerr << "FAILED: " << message << '\n'
                  << "Expected: " << expected << '\n'
                  << "Actual: " << actual << '\n'
                  << "Tolerance: " << tolerance << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void expectTrue(
    bool condition,
    std::string_view message
) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

template <typename Func>
void expectThrows(
    Func&& func,
    std::string_view message
) {
    try {
        func();
    } catch (const std::invalid_argument&) {
        return;
    }

    std::cerr << "FAILED: " << message
              << " - expected std::invalid_argument\n";

    std::exit(EXIT_FAILURE);
}

} // namespace

int main() {
    

    // ------------------------------------------------------------
    // Test 1: Single long position
    // ------------------------------------------------------------

    {
        Portfolio portfolio;

        portfolio.addPosition({
            10.0,       // quantity
            100.0,      // entry price
            100.0,      // current price
            "AAPL"
        });

        HistoricalPriceSeries history{
            "AAPL",
            {
                "2026-01-01",
                "2026-01-02",
                "2026-01-03",
                "2026-01-04"
            },
            {
                100.0,
                110.0,
                99.0,
                108.9
            }
        };

        PortfolioHistoricalRisk risk;

        const PortfolioHistoricalRiskResult result =
            risk.calculate(
                portfolio,
                {history},
                0.75
            );

        expectNear(
            result.currentMarketValue,
            1000.0,
            1e-12,
            "Single long current market value"
        );

        expectNear(
            result.currentPnl,
            0.0,
            1e-12,
            "Single long current P&L"
        );

        expectTrue(
            result.scenarioPnl.size() == 3,
            "Single long should produce three scenarios"
        );

        // Returns:
        //
        // 100 -> 110 = +10%
        // 110 -> 99  = -10%
        // 99  -> 108.9 = +10%
        //
        // Position value = 10 * 100 = 1000
        //
        // Scenario P&L:
        // +100, -100, +100

        expectNear(
            result.scenarioPnl[0],
            100.0,
            1e-12,
            "Single long scenario 1 P&L"
        );

        expectNear(
            result.scenarioPnl[1],
            -100.0,
            1e-12,
            "Single long scenario 2 P&L"
        );

        expectNear(
            result.scenarioPnl[2],
            100.0,
            1e-12,
            "Single long scenario 3 P&L"
        );
    }

    // ------------------------------------------------------------
    // Test 2: Single short position
    // ------------------------------------------------------------

    {
        Portfolio portfolio;

        portfolio.addPosition({
            -10.0,
            100.0,
            100.0,
            "AAPL"
        });

        HistoricalPriceSeries history{
            "AAPL",
            {
                "2026-01-01",
                "2026-01-02",
                "2026-01-03"
            },
            {
                100.0,
                110.0,
                99.0
            }
        };

        PortfolioHistoricalRisk risk;

        const PortfolioHistoricalRiskResult result =
            risk.calculate(
                portfolio,
                {history},
                0.50
            );

        // Short position:
        //
        // +10% underlying return
        // -> -$100 P&L
        //
        // -10% underlying return
        // -> +$100 P&L

        expectNear(
            result.scenarioPnl[0],
            -100.0,
            1e-12,
            "Short position positive-price-move P&L"
        );

        expectNear(
            result.scenarioPnl[1],
            100.0,
            1e-12,
            "Short position negative-price-move P&L"
        );
    }

    // ------------------------------------------------------------
    // Test 3: Two correlated assets
    // ------------------------------------------------------------

    {
        Portfolio portfolio;

        portfolio.addPosition({
            10.0,
            100.0,
            100.0,
            "ASSET_A"
        });

        portfolio.addPosition({
            20.0,
            50.0,
            50.0,
            "ASSET_B"
        });

        HistoricalPriceSeries assetA{
            "ASSET_A",
            {
                "2026-01-01",
                "2026-01-02",
                "2026-01-03",
                "2026-01-04"
            },
            {
                100.0,
                110.0,
                99.0,
                108.9
            }
        };

        HistoricalPriceSeries assetB{
            "ASSET_B",
            {
                "2026-01-01",
                "2026-01-02",
                "2026-01-03",
                "2026-01-04"
            },
            {
                50.0,
                55.0,
                49.5,
                54.45
            }
        };

        PortfolioHistoricalRisk risk;

        const PortfolioHistoricalRiskResult result =
            risk.calculate(
                portfolio,
                {assetA, assetB},
                0.75
            );

        // Both assets have identical returns:
        //
        // +10%, -10%, +10%
        //
        // Asset A exposure = 10 * 100 = $1000
        // Asset B exposure = 20 * 50  = $1000
        //
        // Total portfolio exposure = $2000
        //
        // Portfolio scenarios:
        //
        // +10% -> +$200
        // -10% -> -$200
        // +10% -> +$200

        expectNear(
            result.scenarioPnl[0],
            200.0,
            1e-12,
            "Correlated assets scenario 1 P&L"
        );

        expectNear(
            result.scenarioPnl[1],
            -200.0,
            1e-12,
            "Correlated assets scenario 2 P&L"
        );

        expectNear(
            result.scenarioPnl[2],
            200.0,
            1e-12,
            "Correlated assets scenario 3 P&L"
        );
    }

    // Test 4: Mismatched date grids must throw
    {
        Portfolio portfolio;

        portfolio.addPosition({
            1.0,
            100.0,
            100.0,
            "ASSET_A"
        });

        portfolio.addPosition({
            1.0,
            100.0,
            100.0,
            "ASSET_B"
        });

        HistoricalPriceSeries assetA{
            "ASSET_A",
            {
                "2026-01-01",
                "2026-01-02",
                "2026-01-03",
                "2026-01-04"
            },
            {
                100.0,
                110.0,
                105.0,
                115.0
            }
        };

        HistoricalPriceSeries assetB{
            "ASSET_B",
            {
                "2026-01-01",
                "2026-01-03",
                "2026-01-04"
            },
            {
                100.0,
                105.0,
                115.0
            }
        };
        PortfolioHistoricalRisk risk;

        expectThrows(
            [&]() {
                risk.calculate(
                    portfolio,
                    {assetA, assetB},
                    0.95
                );
            },
            "Portfolio histories with different date grids should throw"
        );
    }

    // ------------------------------------------------------------
    // Test 5: Currency VaR
    // ------------------------------------------------------------

    {
        Portfolio portfolio;

        portfolio.addPosition({
            10.0,
            100.0,
            100.0,
            "A"
        });

        HistoricalPriceSeries history{
            "A",
            {
                "2026-01-01",
                "2026-01-02",
                "2026-01-03",
                "2026-01-04",
                "2026-01-05"
            },
            {
                100.0,
                110.0,
                90.0,
                100.0,
                95.0
            }
        };

        PortfolioHistoricalRisk risk;

        const PortfolioHistoricalRiskResult result =
            risk.calculate(
                portfolio,
                {history},
                0.75
            );

        // Returns:
        //
        // +10%
        // -18.1818%
        // +11.1111%
        // -5%
        //
        // Scenario P&L:
        //
        // +100
        // -181.818...
        // +111.111...
        // -50
        //
        // 75% confidence -> 25th percentile.
        //
        // Sorted:
        //
        // -181.818...
        // -50
        // +100
        // +111.111...
        //
        // Position = 0.25 * (4 - 1) = 0.75
        //
        // Quantile:
        //
        // -181.818 + 0.75 * ( -50 - (-181.818) )
        // = -83.636...
        //
        // VaR = 83.636...

        expectNear(
        result.var,
        82.95454545454545,
        1e-9,
        "Portfolio historical VaR in currency"
    );
    }

    // ------------------------------------------------------------
    // Test 6: Currency Expected Shortfall
    // ------------------------------------------------------------

    {
        Portfolio portfolio;

        portfolio.addPosition({
            10.0,
            100.0,
            100.0,
            "A"
        });

        HistoricalPriceSeries history{
            "A",
            {
                "2026-01-01",
                "2026-01-02",
                "2026-01-03",
                "2026-01-04",
                "2026-01-05"
            },
            {
                100.0,
                110.0,
                90.0,
                100.0,
                95.0
            }
        };

        PortfolioHistoricalRisk risk;

        const PortfolioHistoricalRiskResult result =
            risk.calculate(
                portfolio,
                {history},
                0.75
            );

        // Tail size:
        //
        // (1 - 0.75) * 4 = 1
        //
        // Worst scenario:
        //
        // -$181.818...
        //
        // Therefore:
        //
        // ES = $181.818...

        expectNear(
            result.expectedShortfall,
            181.8181818181818,
            1e-9,
            "Portfolio historical Expected Shortfall in currency"
        );
    }

    // ------------------------------------------------------------
    // Test 7: Missing historical series
    // ------------------------------------------------------------

    {
        Portfolio portfolio;

        portfolio.addPosition({
            10.0,
            100.0,
            100.0,
            "A"
        });

        HistoricalPriceSeries wrongHistory{
            "B",
            {
                "2026-01-01",
                "2026-01-02"
            },
            {
                100.0,
                110.0
            }
        };

        PortfolioHistoricalRisk risk;

        expectThrows(
            [&]() {
                risk.calculate(
                    portfolio,
                    {wrongHistory},
                    0.95
                );
            },
            "Missing historical series should throw"
        );
    }

    // ------------------------------------------------------------
    // Test 8: Invalid confidence level
    // ------------------------------------------------------------

    {
        Portfolio portfolio;

        portfolio.addPosition({
            10.0,
            100.0,
            100.0,
            "A"
        });

        HistoricalPriceSeries history{
            "A",
            {
                "2026-01-01",
                "2026-01-02"
            },
            {
                100.0,
                110.0
            }
        };

        PortfolioHistoricalRisk risk;

        expectThrows(
            [&]() {
                risk.calculate(
                    portfolio,
                    {history},
                    0.0
                );
            },
            "Zero confidence level should throw"
        );

        expectThrows(
            [&]() {
                risk.calculate(
                    portfolio,
                    {history},
                    1.0
                );
            },
            "One confidence level should throw"
        );
    }

    std::cout
        << "All Portfolio Historical Risk tests passed.\n";

    return EXIT_SUCCESS;
}