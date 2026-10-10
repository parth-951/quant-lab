#include "risk_comparison.hpp"
#include "parametric_var.hpp"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

bool near(double actual, double expected, double tolerance = 1e-6) {
    return std::abs(actual - expected) <= tolerance;
}

template <typename Function>
void requireThrows(Function&& function, const std::string& message) {
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
        const std::vector<double> returns{
            -0.03, -0.02, -0.01, 0.0, 0.01,
             0.02,  0.03,  0.01, -0.01, 0.0
        };

        const ql::CovarianceMatrix::Matrix covariance{
            {0.0001, 0.0},
            {0.0, 0.0004}
        };

        const std::vector<double> weights{0.6, 0.4};
        constexpr double portfolioValue = 1'000'000.0;
        constexpr double confidence = 0.8;

        // 1. Basic risk comparison.
        const auto result = ql::RiskComparison::calculate(
            returns, portfolioValue, weights, covariance, confidence);

        require(result.historicalVaR >= 0.0,
                "Historical VaR should be nonnegative");
        require(result.historicalES >= 0.0,
                "Historical ES should be nonnegative");
        require(result.parametricVaR > 0.0,
                "Parametric VaR should be positive");
        require(result.parametricES > 0.0,
                "Parametric ES should be positive");
        require(result.portfolioValue == portfolioValue,
                "Portfolio value should be preserved");
        require(result.confidenceLevel == confidence,
                "Confidence level should be preserved");
        require(result.horizonDays == 1,
                "Default horizon should be one day");

        // 2. Parametric VaR agrees with the standalone implementation.
        const double expectedVaR = ql::ParametricVaR::calculate(
            portfolioValue, weights, covariance, confidence);

        require(near(result.parametricVaR, expectedVaR),
                "Parametric VaR should match standalone calculation");

        // 3. Two-day historical returns should use rolling compounding.
        const auto twoDay = ql::RiskComparison::calculate(
            returns, portfolioValue, weights, covariance, confidence, 2);

        require(twoDay.horizonDays == 2,
                "Two-day horizon should be preserved");
        require(twoDay.historicalVaR >= 0.0,
                "Two-day historical VaR should be nonnegative");
        require(twoDay.historicalES >= 0.0,
                "Two-day historical ES should be nonnegative");

        // 4. Zero portfolio value should produce zero monetary estimates.
        const auto zeroValue = ql::RiskComparison::calculate(
            returns, 0.0, weights, covariance, confidence);

        require(near(zeroValue.historicalVaR, 0.0),
                "Zero portfolio value should produce zero historical VaR");
        require(near(zeroValue.historicalES, 0.0),
                "Zero portfolio value should produce zero historical ES");
        require(near(zeroValue.parametricVaR, 0.0),
                "Zero portfolio value should produce zero parametric VaR");
        require(near(zeroValue.parametricES, 0.0),
                "Zero portfolio value should produce zero parametric ES");

        // 5. Invalid input checks.
        requireThrows([&] {
            (void)ql::RiskComparison::calculate(
                {}, portfolioValue, weights, covariance, confidence);
        }, "Empty returns should throw");

        requireThrows([&] {
            (void)ql::RiskComparison::calculate(
                returns, -1.0, weights, covariance, confidence);
        }, "Negative portfolio value should throw");

        requireThrows([&] {
            (void)ql::RiskComparison::calculate(
                returns, std::numeric_limits<double>::infinity(),
                weights, covariance, confidence);
        }, "Infinite portfolio value should throw");

        requireThrows([&] {
            (void)ql::RiskComparison::calculate(
                returns, portfolioValue, weights, covariance, 0.5);
        }, "Confidence of 0.5 should throw");

        requireThrows([&] {
            (void)ql::RiskComparison::calculate(
                returns, portfolioValue, weights, covariance, 1.0);
        }, "Confidence of 1.0 should throw");

        requireThrows([&] {
            (void)ql::RiskComparison::calculate(
                returns, portfolioValue, weights, covariance,
                std::numeric_limits<double>::quiet_NaN());
        }, "NaN confidence should throw");

        requireThrows([&] {
            (void)ql::RiskComparison::calculate(
                returns, portfolioValue, weights, covariance, confidence, 0);
        }, "Zero horizon should throw");

        requireThrows([&] {
            (void)ql::RiskComparison::calculate(
                returns, portfolioValue, weights, covariance,
                confidence, returns.size() + 1);
        }, "Horizon longer than history should throw");

        requireThrows([&] {
            (void)ql::RiskComparison::calculate(
                {0.01, -1.0, 0.02}, portfolioValue,
                weights, covariance, confidence);
        }, "Return of -100% should throw");

        requireThrows([&] {
            (void)ql::RiskComparison::calculate(
                {0.01, std::numeric_limits<double>::quiet_NaN()},
                portfolioValue, weights, covariance, confidence);
        }, "NaN return should throw");

        requireThrows([&] {
            (void)ql::RiskComparison::calculate(
                returns, portfolioValue, {0.6, 0.3},
                covariance, confidence);
        }, "Unnormalized weights should throw");

        requireThrows([&] {
            (void)ql::RiskComparison::calculate(
                returns, portfolioValue,
                {0.6, std::numeric_limits<double>::infinity()},
                covariance, confidence);
        }, "Non-finite weights should throw");

        requireThrows([&] {
            (void)ql::RiskComparison::calculate(
                returns, portfolioValue, weights,
                {{0.0001}}, confidence);
        }, "Covariance dimension mismatch should throw");

        std::cout << "All risk comparison tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Risk comparison test failure: "
                  << error.what() << '\n';
        return 1;
    }
}