#include "portfolio.hpp"
#include "position.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {

void expectNear(
    std::string_view name,
    double actual,
    double expected,
    double tolerance
) {
    if (std::abs(actual - expected) > tolerance) {
        std::cerr
            << "FAILED: " << name
            << "\nExpected: " << expected
            << "\nActual: " << actual
            << "\nTolerance: " << tolerance
            << "\n";

        std::exit(EXIT_FAILURE);
    }
}

void expectTrue(
    std::string_view name,
    bool condition
) {
    if (!condition) {
        std::cerr
            << "FAILED: " << name
            << "\n";

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
            << "\nExpected invalid_argument exception.\n";

        std::exit(EXIT_FAILURE);
    }
    catch (const std::invalid_argument&) {
        // Expected.
    }
}

} // namespace

int main() {

    // ========================================================
    // POSITION TESTS
    // ========================================================

    // Long position
    const ql::Position longPosition{
        100.0,
        100.0,
        150.0,
        "LONG_TEST"
    };

    expectNear(
        "Long position market value",
        longPosition.marketValue(),
        15000.0,
        1e-12
    );

    expectNear(
        "Long position P&L",
        longPosition.pnl(),
        5000.0,
        1e-12
    );

    expectNear(
        "Long position P&L percentage",
        longPosition.pnlPercent(),
        0.50,
        1e-12
    );


    // Short position
    const ql::Position shortPosition{
        -100.0,
        150.0,
        100.0,
        "SHORT_TEST"
    };

    expectNear(
        "Short position market value",
        shortPosition.marketValue(),
        -10000.0,
        1e-12
    );

    expectNear(
        "Short position P&L",
        shortPosition.pnl(),
        5000.0,
        1e-12
    );


    // Fractional quantity
    const ql::Position fractionalPosition{
        2.5,
        80.0,
        100.0,
        "FRACTIONAL_TEST"
    };

    expectNear(
        "Fractional position market value",
        fractionalPosition.marketValue(),
        250.0,
        1e-12
    );

    expectNear(
        "Fractional position P&L",
        fractionalPosition.pnl(),
        50.0,
        1e-12
    );


    // Zero quantity
    const ql::Position emptyPosition{
        0.0,
        100.0,
        150.0,
        "EMPTY_TEST"
    };

    expectNear(
        "Zero quantity market value",
        emptyPosition.marketValue(),
        0.0,
        1e-12
    );

    expectNear(
        "Zero quantity P&L",
        emptyPosition.pnl(),
        0.0,
        1e-12
    );


    // ========================================================
    // POSITION VALIDATION
    // ========================================================

    // NaN quantity
    expectThrows(
        "Portfolio rejects NaN quantity",
        []() {
            ql::Portfolio portfolio;

            portfolio.addPosition(
                ql::Position{
                    std::nan(""),
                    100.0,
                    110.0,
                    "NAN_QTY"
                }
            );
        }
    );


    // Negative entry price
    expectThrows(
        "Portfolio rejects negative entry price",
        []() {
            ql::Portfolio portfolio;

            portfolio.addPosition(
                ql::Position{
                    100.0,
                    -100.0,
                    110.0,
                    "NEG_ENTRY"
                }
            );
        }
    );


    // Zero current price
    expectThrows(
        "Portfolio rejects zero current price",
        []() {
            ql::Portfolio portfolio;

            portfolio.addPosition(
                ql::Position{
                    100.0,
                    100.0,
                    0.0,
                    "ZERO_CURRENT"
                }
            );
        }
    );


    // Empty instrument ID
    expectThrows(
        "Portfolio rejects empty instrument ID",
        []() {
            ql::Portfolio portfolio;

            portfolio.addPosition(
                ql::Position{
                    100.0,
                    100.0,
                    110.0,
                    ""
                }
            );
        }
    );


// Test: Portfolio allows multiple lots of the same instrument
{
    ql::Portfolio portfolio;

    portfolio.addPosition({
        10.0,
        100.0,
        110.0,
        "AAPL"
    });

    portfolio.addPosition({
        5.0,
        105.0,
        110.0,
        "AAPL"
    });

    expectTrue(
        "Portfolio should allow multiple lots of the same instrument",
        portfolio.positions().size() == 2
    );

    expectNear(
        "Portfolio market value with multiple lots",
        portfolio.marketValue(),
        1650.0,
        1e-12
    );

    expectNear(
        "Portfolio P&L with multiple lots",
        portfolio.pnl(),
        125.0,
        1e-12
    );
}

    // ========================================================
    // PORTFOLIO TESTS
    // ========================================================

    // Empty portfolio
    const ql::Portfolio emptyPortfolio;

    expectNear(
        "Empty portfolio market value",
        emptyPortfolio.marketValue(),
        0.0,
        1e-12
    );

    expectNear(
        "Empty portfolio P&L",
        emptyPortfolio.pnl(),
        0.0,
        1e-12
    );


    // Single position
    ql::Portfolio singlePositionPortfolio;

    singlePositionPortfolio.addPosition(
        ql::Position{
            100.0,
            100.0,
            150.0,
            "SINGLE_TEST"
        }
    );

    expectNear(
        "Single position portfolio value",
        singlePositionPortfolio.marketValue(),
        15000.0,
        1e-12
    );

    expectNear(
        "Single position portfolio P&L",
        singlePositionPortfolio.pnl(),
        5000.0,
        1e-12
    );


    // Multiple long positions
    ql::Portfolio multiPositionPortfolio;

    multiPositionPortfolio.addPosition(
        ql::Position{
            100.0,
            100.0,
            150.0,
            "STOCK_A"
        }
    );

    multiPositionPortfolio.addPosition(
        ql::Position{
            50.0,
            150.0,
            200.0,
            "STOCK_B"
        }
    );

    expectNear(
        "Multiple position portfolio value",
        multiPositionPortfolio.marketValue(),
        25000.0,
        1e-12
    );

    expectNear(
        "Multiple position portfolio P&L",
        multiPositionPortfolio.pnl(),
        7500.0,
        1e-12
    );


    // Long + short positions
    ql::Portfolio longShortPortfolio;

    longShortPortfolio.addPosition(
        ql::Position{
            100.0,
            100.0,
            150.0,
            "LONG"
        }
    );

    longShortPortfolio.addPosition(
        ql::Position{
            -50.0,
            100.0,
            60.0,
            "SHORT"
        }
    );

    expectNear(
        "Long short portfolio value",
        longShortPortfolio.marketValue(),
        12000.0,
        1e-12
    );

    expectNear(
        "Long short portfolio P&L",
        longShortPortfolio.pnl(),
        7000.0,
        1e-12
    );


    // Portfolio position count
    expectTrue(
        "Portfolio contains two positions",
        longShortPortfolio.positions().size() == 2
    );


    // ========================================================
    // RESULTS
    // ========================================================

    std::cout << "All portfolio tests passed.\n";

    return 0;
}
