#include "portfolio.hpp"
#include "position.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
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

} // namespace

int main() {

    // ========================================================
    // POSITION TESTS
    // ========================================================

    // Long position
    const ql::Position longPosition{
        100.0,
        150.0
    };

    expectNear(
        "Long position market value",
        longPosition.marketValue(),
        15000.0,
        1e-12
    );


    // Short position
    const ql::Position shortPosition{
        -100.0,
        150.0
    };

    expectNear(
        "Short position market value",
        shortPosition.marketValue(),
        -15000.0,
        1e-12
    );


    // Fractional quantity
    const ql::Position fractionalPosition{
        2.5,
        100.0
    };

    expectNear(
        "Fractional position market value",
        fractionalPosition.marketValue(),
        250.0,
        1e-12
    );


    // Zero quantity
    const ql::Position emptyPosition{
        0.0,
        100.0
    };

    expectNear(
        "Zero quantity market value",
        emptyPosition.marketValue(),
        0.0,
        1e-12
    );


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


    // Single position
    ql::Portfolio singlePositionPortfolio;

    singlePositionPortfolio.addPosition(
        ql::Position{100.0, 150.0}
    );

    expectNear(
        "Single position portfolio value",
        singlePositionPortfolio.marketValue(),
        15000.0,
        1e-12
    );


    // Multiple long positions
    ql::Portfolio multiPositionPortfolio;

    multiPositionPortfolio.addPosition(
        ql::Position{100.0, 150.0}
    );

    multiPositionPortfolio.addPosition(
        ql::Position{50.0, 200.0}
    );

    expectNear(
        "Multiple position portfolio value",
        multiPositionPortfolio.marketValue(),
        25000.0,
        1e-12
    );


    // Long + short positions
    ql::Portfolio longShortPortfolio;

    longShortPortfolio.addPosition(
        ql::Position{100.0, 150.0}
    );

    longShortPortfolio.addPosition(
        ql::Position{-50.0, 100.0}
    );

    expectNear(
        "Long short portfolio value",
        longShortPortfolio.marketValue(),
        10000.0,
        1e-12
    );


    // Portfolio position count
    expectTrue(
        "Portfolio contains two positions",
        longShortPortfolio.positions().size() == 2
    );


    std::cout << "All portfolio tests passed.\n";

    return 0;
}