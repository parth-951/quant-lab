#include "historical_risk.hpp"
#include "historical_var.hpp"

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

void expectTrue(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

template <typename Func>
void expectThrows(Func&& func, std::string_view message) {
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
    HistoricalRisk risk;

    // ------------------------------------------------------------
    // Test 1: Prices -> returns
    // ------------------------------------------------------------

    {
        const std::vector<double> prices{
            100.0,
            110.0,
            99.0,
            108.9
        };

        const HistoricalRiskResult result =
            risk.calculate(prices, 0.95);

        expectTrue(
            result.returns.size() == 3,
            "HistoricalRisk should generate N-1 returns"
        );

        expectNear(
            result.returns[0],
            0.10,
            1e-12,
            "First historical return"
        );

        expectNear(
            result.returns[1],
            -0.10,
            1e-12,
            "Second historical return"
        );

        expectNear(
            result.returns[2],
            0.10,
            1e-12,
            "Third historical return"
        );
    }

    // ------------------------------------------------------------
    // Test 2: VaR and Expected Shortfall should be non-negative
    // ------------------------------------------------------------

    {
        const std::vector<double> prices{
            100.0,
            110.0,
            99.0,
            108.9,
            98.01,
            107.811,
            97.0299
        };

        const HistoricalRiskResult result =
            risk.calculate(prices, 0.95);

        expectTrue(
            result.var >= 0.0,
            "Historical VaR should be non-negative"
        );

        expectTrue(
            result.expectedShortfall >= 0.0,
            "Expected Shortfall should be non-negative"
        );

        expectTrue(
            result.expectedShortfall >= result.var,
            "Expected Shortfall should be greater than or equal to VaR"
        );
    }

    // ------------------------------------------------------------
    // Test 3: VaR should be independent of return ordering
    //
    // IMPORTANT:
    // We test the return series directly here.
    //
    // Reversing a PRICE series changes the returns, so reversing
    // prices should NOT be expected to produce the same VaR.
    // ------------------------------------------------------------

    {
        HistoricalVaR var;

        const std::vector<double> returnsA{
            -0.10,
            -0.05,
            -0.02,
             0.01,
             0.03,
             0.05,
             0.08,
             0.12,
             0.15,
             0.20
        };

        const std::vector<double> returnsB{
             0.12,
            -0.02,
             0.20,
            -0.10,
             0.05,
             0.15,
            -0.05,
             0.08,
             0.01,
             0.03
        };

        const double varA =
            var.calculate(returnsA, 0.95);

        const double varB =
            var.calculate(returnsB, 0.95);

        expectNear(
            varA,
            varB,
            1e-12,
            "Historical VaR should be independent of return ordering"
        );
    }

    // ------------------------------------------------------------
    // Test 4: Invalid price series
    // ------------------------------------------------------------

    {
        expectThrows(
            [&]() {
                risk.calculate({}, 0.95);
            },
            "Empty price series should throw"
        );
    }

    // ------------------------------------------------------------
    // Test 5: Price series containing zero
    // ------------------------------------------------------------

    {
        const std::vector<double> prices{
            100.0,
            0.0,
            110.0
        };

        expectThrows(
            [&]() {
                risk.calculate(prices, 0.95);
            },
            "Zero price should throw"
        );
    }

    // ------------------------------------------------------------
    // Test 6: Negative price
    // ------------------------------------------------------------

    {
        const std::vector<double> prices{
            100.0,
            -10.0,
            110.0
        };

        expectThrows(
            [&]() {
                risk.calculate(prices, 0.95);
            },
            "Negative price should throw"
        );
    }

    // ------------------------------------------------------------
    // Test 7: Invalid confidence level
    // ------------------------------------------------------------

    {
        const std::vector<double> prices{
            100.0,
            105.0,
            98.0,
            110.0
        };

        expectThrows(
            [&]() {
                risk.calculate(prices, 0.0);
            },
            "Confidence level of 0 should throw"
        );

        expectThrows(
            [&]() {
                risk.calculate(prices, 1.0);
            },
            "Confidence level of 1 should throw"
        );

        expectThrows(
            [&]() {
                risk.calculate(prices, -0.95);
            },
            "Negative confidence level should throw"
        );

        expectThrows(
            [&]() {
                risk.calculate(prices, 1.5);
            },
            "Confidence level above 1 should throw"
        );
    }

    // ------------------------------------------------------------
    // Test 8: Expected Shortfall should be at least VaR
    // ------------------------------------------------------------

    {
        const std::vector<double> prices{
            100.0,
            105.0,
            110.0,
            99.0,
            95.0,
            90.0,
            92.0,
            85.0,
            100.0,
            80.0,
            120.0
        };

        const HistoricalRiskResult result =
            risk.calculate(prices, 0.80);

        expectTrue(
            result.expectedShortfall >= result.var,
            "Expected Shortfall should be at least VaR"
        );
    }

    // ------------------------------------------------------------
    // Test 9: Expected Shortfall exact hand-calculated value
    //
    // Returns:
    //   -10%, -5%, -2%, +1%, +3%
    //
    // Confidence = 80%
    //
    // Tail size:
    //   (1 - 0.80) * 5 = 1 observation
    //
    // Worst 1 observation:
    //   -10%
    //
    // Therefore:
    //   ES = 10%
    // ------------------------------------------------------------

    {
        const std::vector<double> prices{
            100.0,
            90.0,
            85.5,
            83.79,
            84.6279,
            87.166737
        };

        const HistoricalRiskResult result =
            risk.calculate(prices, 0.80);

        expectNear(
            result.expectedShortfall,
            0.10,
            1e-12,
            "Expected Shortfall exact hand-calculated value"
        );
    }

    std::cout << "All Historical Risk tests passed.\n";

    return EXIT_SUCCESS;
}