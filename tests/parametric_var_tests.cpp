#include "parametric_var.hpp"
#include "portfolio_volatility.hpp"

#include <cmath>
#include <exception>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

int failures = 0;

void expectNear(
    const std::string& name,
    double actual,
    double expected,
    double tolerance = 1e-8) {

    if (!std::isfinite(actual) ||
        std::abs(actual - expected) > tolerance) {
        std::cerr << "FAIL: " << name
                  << " (actual=" << actual
                  << ", expected=" << expected << ")\n";
        ++failures;
    } else {
        std::cout << "PASS: " << name << '\n';
    }
}

template <typename Function>
void expectThrows(const std::string& name, Function function) {
    try {
        function();
        std::cerr << "FAIL: " << name << " (no exception thrown)\n";
        ++failures;
    } catch (const std::exception&) {
        std::cout << "PASS: " << name << '\n';
    }
}

const std::vector<double> weights{0.6, 0.4};

const ql::CovarianceMatrix::Matrix covariance{
    {0.0001, 0.0},
    {0.0,    0.0004}
};

double calculate(
    double value = 1'000'000.0,
    double confidence = 0.95,
    std::size_t days = 1) {

    return ql::ParametricVaR::calculate(
        value, weights, covariance, confidence, days);
}

} // namespace

int main() {
    constexpr double portfolioValue = 1'000'000.0;
    constexpr double dailyVolatility = 0.01;
    constexpr double z95 = 1.6448536269514722;
    constexpr double z99 = 2.3263478740408408;

    expectNear(
        "Portfolio volatility from existing module",
        ql::PortfolioVolatility::volatility(weights, covariance),
        dailyVolatility);

    // VaR is a large monetary amount, so allow a small absolute tolerance
    // for the inverse-normal approximation's floating-point error.
    expectNear(
        "One-day 95% VaR",
        calculate(),
        portfolioValue * dailyVolatility * z95,
        1e-3);

    expectNear(
        "One-day 99% VaR",
        calculate(portfolioValue, 0.99),
        portfolioValue * dailyVolatility * z99,
        1e-3);

    expectNear(
        "Four-day VaR scales by square root of time",
        calculate(portfolioValue, 0.95, 4),
        2.0 * calculate(),
        1e-6);

    expectNear(
        "VaR scales linearly with portfolio value",
        calculate(2.0 * portfolioValue),
        2.0 * calculate(),
        1e-6);

    expectNear(
        "Zero portfolio value produces zero VaR",
        calculate(0.0),
        0.0);

    const ql::CovarianceMatrix::Matrix zeroCovariance{
        {0.0, 0.0},
        {0.0, 0.0}
    };

    expectNear(
        "Zero volatility produces zero VaR",
        ql::ParametricVaR::calculate(
            portfolioValue, weights, zeroCovariance, 0.95),
        0.0);

    expectThrows("Reject confidence level of 0.5", [] {
        (void)calculate(portfolioValue, 0.5);
    });

    expectThrows("Reject confidence level of 1.0", [] {
        (void)calculate(portfolioValue, 1.0);
    });

    expectThrows("Reject confidence level below 0.5", [] {
        (void)calculate(portfolioValue, 0.4);
    });

    expectThrows("Reject non-finite confidence level", [] {
        (void)calculate(
            portfolioValue,
            std::numeric_limits<double>::quiet_NaN());
    });

    expectThrows("Reject negative portfolio value", [] {
        (void)calculate(-1.0);
    });

    expectThrows("Reject non-finite portfolio value", [] {
        (void)calculate(std::numeric_limits<double>::infinity());
    });

    expectThrows("Reject zero-day horizon", [] {
        (void)calculate(portfolioValue, 0.95, 0);
    });

    expectThrows("Reject empty weights", [] {
        (void)ql::ParametricVaR::calculate(
            portfolioValue, {}, covariance, 0.95);
    });

    expectThrows("Reject weights/covariance dimension mismatch", [] {
        (void)ql::ParametricVaR::calculate(
            portfolioValue, {1.0}, covariance, 0.95);
    });

    expectThrows("Reject non-finite weight", [] {
        (void)ql::ParametricVaR::calculate(
            portfolioValue,
            {0.6, std::numeric_limits<double>::infinity()},
            covariance,
            0.95);
    });

    expectThrows("Reject non-square covariance matrix", [] {
        const ql::CovarianceMatrix::Matrix invalid{
            {0.0001, 0.0},
            {0.0}
        };

        (void)ql::ParametricVaR::calculate(
            portfolioValue, weights, invalid, 0.95);
    });

    expectThrows("Reject negative portfolio variance", [] {
        const ql::CovarianceMatrix::Matrix invalid{
            {-0.0004, 0.0},
            {0.0, 0.0004}
        };

        (void)ql::ParametricVaR::calculate(
            portfolioValue, weights, invalid, 0.95);
    });

    if (failures != 0) {
        std::cerr << "\n" << failures << " test(s) failed.\n";
        return 1;
    }

    std::cout << "\nAll parametric VaR tests passed.\n";
    return 0;
}
