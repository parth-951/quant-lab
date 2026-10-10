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
    } catch (const std::exception&) {
        threw = true;
    }
    require(threw, message);
}

} // namespace

int main() {
    try {
        // 1. Two-asset example:
        // variance = .6^2*.01 + 2*.6*.4*.002 + .4^2*.04 = .01096
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

        // 2. Single asset: portfolio variance = weight^2 * asset variance.
        const double singleAssetVariance =
            ql::PortfolioVolatility::variance({2.0}, {{0.03}});
        require(near(singleAssetVariance, 0.12),
                "Single-asset variance should scale by squared weight");

        // 3. Zero weights yield zero portfolio variance.
        require(near(
                    ql::PortfolioVolatility::variance({0.0, 0.0}, covariance),
                    0.0),
                "All-zero weights should yield zero variance");

        // 4. Input validation.
        requireThrows([] {
            (void)ql::PortfolioVolatility::variance({}, {});
        }, "Empty weights should throw");

        requireThrows([] {
            (void)ql::PortfolioVolatility::variance({0.5}, {
                {0.01, 0.002},
                {0.002, 0.04}
            });
        }, "Weight/covariance dimension mismatch should throw");

        requireThrows([] {
            (void)ql::PortfolioVolatility::variance({0.5, 0.5}, {
                {0.01, 0.002},
                {0.002}
            });
        }, "Non-square covariance matrix should throw");

        requireThrows([] {
            (void)ql::PortfolioVolatility::variance({
                std::numeric_limits<double>::quiet_NaN()
            }, {{0.01}});
        }, "Non-finite weight should throw");

        requireThrows([] {
            (void)ql::PortfolioVolatility::variance({1.0}, {{
                std::numeric_limits<double>::infinity()
            }});
        }, "Non-finite covariance should throw");

        requireThrows([] {
            (void)ql::PortfolioVolatility::variance({0.5, 0.5}, {
                {0.01, 0.002},
                {0.003, 0.04}
            });
        }, "Asymmetric covariance matrix should throw");

        requireThrows([] {
            (void)ql::PortfolioVolatility::variance({1.0}, {{-0.1}});
        }, "Materially negative variance should throw");

        std::cout << "All portfolio volatility tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Portfolio volatility test failure: "
                  << error.what() << '\n';
        return 1;
    }
}
