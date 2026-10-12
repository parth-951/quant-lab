
#include "portfolio_volatility.hpp"

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

bool near(double actual, double expected, double tolerance = 1e-10) {
    return std::abs(actual - expected) <= tolerance;
}

template <typename Function>
void requireThrows(Function&& function, const std::string& message) {
    bool threw = false;

    try {
        function();
    } catch (const std::invalid_argument&) {
        threw = true;
    } catch (const std::domain_error&) {
        threw = true;
    }

    require(threw, message);
}

} // namespace

int main() {
    try {
        // 1. Two-asset portfolio example.
        const std::vector<double> weights{0.6, 0.4};
        const ql::CovarianceMatrix::Matrix covariance{
            {0.01, 0.002},
            {0.002, 0.04}
        };

        const double variance =
            ql::PortfolioVolatility::variance(weights, covariance);

        const double volatility =
            ql::PortfolioVolatility::volatility(weights, covariance);

        require(near(variance, 0.01096),
                "Portfolio variance should match the hand-calculated value");

        require(near(volatility, std::sqrt(0.01096)),
                "Portfolio volatility should be sqrt of variance");

        // 2. Single asset.
        require(
            near(ql::PortfolioVolatility::variance({2.0}, {{0.03}}), 0.12),
            "Single-asset variance should scale by squared weight");

        // 3. Zero weights produce zero variance.
        require(
            near(ql::PortfolioVolatility::variance({0.0, 0.0}, covariance), 0.0),
            "All-zero weights should yield zero variance");

        // 4. Empty weights must be rejected.
        requireThrows([] {
            (void)ql::PortfolioVolatility::variance({}, {});
        }, "Empty weights should throw");

        // 5. Weight/covariance dimension mismatch.
        requireThrows([] {
            (void)ql::PortfolioVolatility::variance(
                {0.5}, {{0.01, 0.002}, {0.002, 0.04}});
        }, "Weight/covariance dimension mismatch should throw");

        // 6. A non-square covariance matrix must be rejected.
        requireThrows([] {
            (void)ql::PortfolioVolatility::variance(
                {0.5, 0.5}, {{0.01, 0.002}, {0.002}});
        }, "Non-square covariance matrix should throw");

        // 7. Malformed row must be rejected before symmetry checking.
        requireThrows([] {
            (void)ql::PortfolioVolatility::variance(
                {0.5, 0.5}, {{1.0, 0.0}, {}});
        }, "Malformed covariance row should throw");

        // 8. First row too short.
        requireThrows([] {
            (void)ql::PortfolioVolatility::variance(
                {0.5, 0.5}, {{1.0}, {0.0, 1.0}});
        }, "Short first covariance row should throw");

        // 9. Non-finite weight must be rejected.
        requireThrows([] {
            (void)ql::PortfolioVolatility::variance(
                {std::numeric_limits<double>::quiet_NaN()}, {{0.01}});
        }, "Non-finite weight should throw");

        // 10. Infinite covariance entry must be rejected.
        requireThrows([] {
            (void)ql::PortfolioVolatility::variance(
                {1.0}, {{std::numeric_limits<double>::infinity()}});
        }, "Non-finite covariance should throw");

        // 11. Asymmetric covariance matrix must be rejected.
        requireThrows([] {
            (void)ql::PortfolioVolatility::variance(
                {0.5, 0.5}, {{0.01, 0.002}, {0.003, 0.04}});
        }, "Asymmetric covariance matrix should throw");

        // 12. Materially negative variance must be rejected.
        requireThrows([] {
            (void)ql::PortfolioVolatility::variance({1.0}, {{-0.1}});
        }, "Materially negative variance should throw");

        // 13. Positive-definite covariance matrix must be accepted.
        const ql::CovarianceMatrix::Matrix positiveDefinite{
            {1.0, 0.2},
            {0.2, 1.0}
        };
        require(
            near(
                ql::PortfolioVolatility::variance(
                    {1.0, 0.0}, positiveDefinite),
                1.0),
            "Positive-definite covariance matrix should be accepted");

        // 14. Singular positive-semidefinite matrix must be accepted.
        const ql::CovarianceMatrix::Matrix singularPsd{
            {1.0, 1.0},
            {1.0, 1.0}
        };
        require(
            near(
                ql::PortfolioVolatility::variance(
                    {0.5, 0.5}, singularPsd),
                1.0),
            "Singular PSD covariance matrix should be accepted");

        // 15. Critical regression: an indefinite matrix must be rejected
        // even when the chosen weights produce a positive variance.
        requireThrows([] {
            (void)ql::PortfolioVolatility::variance(
                {1.0, 0.0}, {{1.0, 2.0}, {2.0, 1.0}});
        }, "Indefinite covariance matrix must be rejected for any weights");

        // 16. A zero covariance matrix is positive semidefinite.
        require(
            near(
                ql::PortfolioVolatility::variance(
                    {0.5, 0.5}, {{0.0, 0.0}, {0.0, 0.0}}),
                0.0),
            "Zero covariance matrix should be accepted");

        // 17. Non-symmetric matrix must be rejected.
        requireThrows([] {
            (void)ql::PortfolioVolatility::variance(
                {1.0, 0.0}, {{1.0, 0.5}, {0.2, 1.0}});
        }, "Non-symmetric covariance matrix should be rejected");

        std::cout << "All portfolio volatility tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Portfolio volatility test failure: "
                  << error.what() << '\n';
        return 1;
    }
}
