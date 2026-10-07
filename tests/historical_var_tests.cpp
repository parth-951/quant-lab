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
    HistoricalVaR var;

    // ------------------------------------------------------------
    // Test 1: Historical VaR with linear interpolation
    // ------------------------------------------------------------

    {
        const std::vector<double> returns{
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

        const double var95 = var.calculate(returns, 0.95);

        // With linear interpolation:
        //
        // position = (1 - 0.95) * (10 - 1)
        //          = 0.45
        //
        // quantile = -0.10 + 0.45 * (-0.05 - (-0.10))
        //          = -0.0775
        //
        // VaR = 0.0775

        expectNear(
            var95,
            0.0775,
            1e-12,
            "Historical VaR 95%"
        );
    }

    // ------------------------------------------------------------
    // Test 2: VaR should be non-negative for a loss distribution
    // ------------------------------------------------------------

    {
        const std::vector<double> returns{
            -0.10,
            -0.08,
            -0.05,
            -0.03,
             0.01,
             0.02,
             0.04,
             0.05
        };

        const double var95 = var.calculate(returns, 0.95);

        expectTrue(
            var95 >= 0.0,
            "Historical VaR should be non-negative"
        );
    }

    // ------------------------------------------------------------
    // Test 3: Return ordering should not affect Historical VaR
    // ------------------------------------------------------------

    {
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

        const double varA = var.calculate(returnsA, 0.95);
        const double varB = var.calculate(returnsB, 0.95);

        expectNear(
            varA,
            varB,
            1e-12,
            "Historical VaR should be independent of return ordering"
        );
    }

    // ------------------------------------------------------------
    // Test 4: Empty returns should throw
    // ------------------------------------------------------------

    {
        expectThrows(
            [&]() {
                var.calculate({}, 0.95);
            },
            "Empty returns should throw"
        );
    }

    // ------------------------------------------------------------
    // Test 5: Invalid confidence level
    // ------------------------------------------------------------

    {
        const std::vector<double> returns{
            -0.10,
            -0.05,
            0.01,
            0.05
        };

        expectThrows(
            [&]() {
                var.calculate(returns, 0.0);
            },
            "Confidence level of 0 should throw"
        );

        expectThrows(
            [&]() {
                var.calculate(returns, 1.0);
            },
            "Confidence level of 1 should throw"
        );

        expectThrows(
            [&]() {
                var.calculate(returns, -0.95);
            },
            "Negative confidence level should throw"
        );

        expectThrows(
            [&]() {
                var.calculate(returns, 1.5);
            },
            "Confidence level above 1 should throw"
        );
    }

    // ------------------------------------------------------------
    // Test 6: Non-finite return should throw
    // ------------------------------------------------------------

    {
        const std::vector<double> returns{
            -0.10,
            std::nan(""),
            0.05
        };

        expectThrows(
            [&]() {
                var.calculate(returns, 0.95);
            },
            "Non-finite return should throw"
        );
    }

    std::cout << "All Historical VaR tests passed.\n";

    return EXIT_SUCCESS;
}