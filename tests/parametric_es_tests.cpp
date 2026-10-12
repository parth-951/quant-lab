
#include "parametric_es.hpp"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

bool near(double actual, double expected, double tolerance = 1e-3) {
    return std::isfinite(actual) &&
           std::abs(actual - expected) <= tolerance;
}

} // namespace

int main() {
    try {
        const ql::CovarianceMatrix::Matrix covariance{
            {0.0001, 0.0},
            {0.0, 0.0004}
        };

        const std::vector<double> weights{0.6, 0.4};
        constexpr double portfolioValue = 1'000'000.0;
        constexpr double confidence = 0.95;
        constexpr double dailyVolatility = 0.01;
        constexpr double z = 1.6448536269514722;
        constexpr double dailyExpectedReturn = 0.001;

        const double pi = std::acos(-1.0);
        const double density =
            std::exp(-0.5 * z * z) / std::sqrt(2.0 * pi);

        // 1. Existing API: zero-drift, one-day ES.
        const double es = ql::ParametricES::calculate(
            portfolioValue, weights, covariance, confidence);

        const double expectedZeroDriftES =
            portfolioValue * dailyVolatility * density /
            (1.0 - confidence);

        require(near(es, expectedZeroDriftES),
                "Zero-drift ES value mismatch");

        // 2. Existing API: four-day square-root-of-time scaling.
        const double fourDayES = ql::ParametricES::calculate(
            portfolioValue, weights, covariance, confidence, 4);

        require(near(fourDayES, 2.0 * es),
                "Zero-drift ES horizon scaling mismatch");

        // 3. Explicit zero drift must preserve the old API's result.
        const double explicitZeroDriftES =
            ql::ParametricES::calculate(
                portfolioValue, weights, covariance,
                confidence, 1, 0.0);

        require(near(explicitZeroDriftES, es),
                "Explicit zero-drift ES should match legacy API");

        // 4. Positive expected return reduces ES.
        const double positiveDriftES =
            ql::ParametricES::calculate(
                portfolioValue, weights, covariance,
                confidence, 1, dailyExpectedReturn);

        const double expectedPositiveDriftES =
            portfolioValue *
            (dailyVolatility * density / (1.0 - confidence)
             - dailyExpectedReturn);

        require(near(positiveDriftES, expectedPositiveDriftES),
                "Positive-drift ES formula mismatch");

        require(positiveDriftES < es,
                "Positive expected return should reduce ES");

        // 5. Negative expected return increases ES.
        const double negativeDriftES =
            ql::ParametricES::calculate(
                portfolioValue, weights, covariance,
                confidence, 1, -dailyExpectedReturn);

        require(negativeDriftES > es,
                "Negative expected return should increase ES");

        // 6. Expected return scales linearly with horizon.
        const double fourDayPositiveDriftES =
            ql::ParametricES::calculate(
                portfolioValue, weights, covariance,
                confidence, 4, dailyExpectedReturn);

        const double expectedFourDayPositiveDriftES =
            portfolioValue *
            (dailyVolatility * std::sqrt(4.0) *
                 density / (1.0 - confidence)
             - dailyExpectedReturn * 4.0);

        require(near(fourDayPositiveDriftES,
                     expectedFourDayPositiveDriftES),
                "ES mean-return horizon scaling mismatch");

        require(fourDayPositiveDriftES < fourDayES,
                "Positive drift should reduce multi-day ES");

        // 7. Reject non-finite expected returns.
        try {
            (void)ql::ParametricES::calculate(
                portfolioValue, weights, covariance,
                confidence, 1,
                std::numeric_limits<double>::quiet_NaN());

            throw std::runtime_error(
                "NaN expected return should be rejected");
        } catch (const std::invalid_argument&) {
            // Expected.
        }

        try {
            (void)ql::ParametricES::calculate(
                portfolioValue, weights, covariance,
                confidence, 1,
                std::numeric_limits<double>::infinity());

            throw std::runtime_error(
                "Infinite expected return should be rejected");
        } catch (const std::invalid_argument&) {
            // Expected.
        }

        // 8. Invalid confidence level is still rejected.
        try {
            (void)ql::ParametricES::calculate(
                100.0, weights, covariance, 1.0);

            throw std::runtime_error(
                "Invalid confidence level should be rejected");
        } catch (const std::invalid_argument&) {
            // Expected.
        }

        // 9. Zero volatility with negative drift still has expected loss.
        const ql::CovarianceMatrix::Matrix zeroCovariance{
            {0.0, 0.0},
            {0.0, 0.0}
        };

        const double zeroVolatilityES =
            ql::ParametricES::calculate(
                100.0, weights, zeroCovariance,
                confidence, 2, -0.01);

        require(near(zeroVolatilityES, 2.0),
                "Negative drift with zero volatility should produce loss");

        // 10. Nonnegative-loss convention for sufficiently positive drift.
        const double nonnegativeES =
            ql::ParametricES::calculate(
                100.0, weights, zeroCovariance,
                confidence, 2, 0.01);

        require(near(nonnegativeES, 0.0),
                "Reported ES should not be negative");

        std::cout << "Parametric ES tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Parametric ES test failure: "
                  << error.what() << '\n';
        return 1;
    }
}
