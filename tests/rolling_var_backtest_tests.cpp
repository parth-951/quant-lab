#include "rolling_var_backtest.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <limits>

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

bool near(
    double actual,
    double expected,
    double tolerance = 1e-10
) {
    return std::abs(actual - expected) <= tolerance;
}

template <typename Function>
void requireThrows(
    Function&& function,
    const std::string& message
) {
    bool threw = false;

    try {
        function();
    } catch (const std::invalid_argument&) {
        threw = true;
    }

    require(threw, message);
}

} // namespace

int main() {
    try {
        // A 2-return lookback generates forecasts for indices 2 and 3.
        const std::vector<double> returns = {
            -0.10, 0.10, -0.20, 0.00
        };

        const auto result = ql::RollingVaRBacktest::run(
            returns,
            2,
            100.0,
            0.75
        );

        require(
            result.predictedVaR.size() == 2,
            "Expected two rolling VaR forecasts"
        );

        require(
            result.realizedLosses.size() == 2,
            "Expected two realized losses"
        );

        require(
            result.forecastIndices == std::vector<std::size_t>{2, 3},
            "Forecast indices should identify the tested days"
        );

        // First window: {-0.10, 0.10}.
        // At 75% confidence, interpolated VaR fraction is 0.05.
        require(
            near(result.predictedVaR[0], 5.0),
            "First forecast should use only the first two returns"
        );

        // Second window: {0.10, -0.20}.
        // Interpolated VaR fraction is 0.125.
        require(
            near(result.predictedVaR[1], 12.5),
            "Second forecast should use only the preceding two returns"
        );

        require(
            near(result.realizedLosses[0], 20.0),
            "First realized loss should be 20"
        );

        require(
            near(result.realizedLosses[1], 0.0),
            "A zero return should produce zero loss"
        );

        require(
            result.backtest.observations == 2,
            "Backtest observation count is incorrect"
        );

        require(
            result.backtest.breaches == 1,
            "Expected exactly one VaR breach"
        );

        require(
            result.backtest.breachIndices ==
                std::vector<std::size_t>{0},
            "Expected the first rolling forecast to breach"
        );

        require(
            near(result.backtest.expectedBreachRate, 0.25),
            "Expected breach rate should be 25%"
        );

        // Invalid lookback window.
        requireThrows([] {
            (void)ql::RollingVaRBacktest::run(
                {-0.01, 0.02},
                0,
                100.0,
                0.95
            );
        }, "Zero lookback should be rejected");

        // Must have at least one observation after the lookback.
        requireThrows([] {
            (void)ql::RollingVaRBacktest::run(
                {-0.01, 0.02},
                2,
                100.0,
                0.95
            );
        }, "Insufficient returns should be rejected");

        requireThrows([] {
            (void)ql::RollingVaRBacktest::run(
                {-0.01, 0.02, 0.03},
                2,
                -100.0,
                0.95
            );
        }, "Negative portfolio value should be rejected");

        requireThrows([] {
            (void)ql::RollingVaRBacktest::run(
                {-0.01, 0.02, 0.03},
                2,
                100.0,
                1.0
            );
        }, "Invalid confidence level should be rejected");

        requireThrows([] {
            (void)ql::RollingVaRBacktest::run(
                {-0.01, 0.02, 0.03},
                2,
                100.0,
                0.95,
                0.0
            );
        }, "Invalid significance level should be rejected");

        requireThrows([] {
            (void)ql::RollingVaRBacktest::run(
                {-0.01, 0.02,
                 std::numeric_limits<double>::quiet_NaN()},
                2,
                100.0,
                0.95
            );
        }, "Non-finite return should be rejected");

        std::cout << "All rolling VaR backtest tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Rolling VaR backtest test failure: "
                  << error.what() << '\n';
        return 1;
    }
}