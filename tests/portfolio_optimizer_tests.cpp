#include "portfolio_optimizer.hpp"

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

bool near(double actual, double expected, double tolerance = 1e-7) {
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

void checkWeights(const std::vector<double>& weights) {
    double sum = 0.0;
    for (double weight : weights) {
        require(weight >= -1e-10 && weight <= 1.0 + 1e-10,
                "Long-only weights must lie in [0, 1]");
        sum += weight;
    }
    require(near(sum, 1.0, 1e-8), "Weights must sum to one");
}

} // namespace

int main() {
    try {
        // 1. For independent assets with variances 1% and 4%, the
        // minimum-variance allocation is 80% / 20%.
        const ql::CovarianceMatrix::Matrix diagonalCovariance{
            {0.01, 0.0},
            {0.0, 0.04}
        };
        const auto result =
            ql::PortfolioOptimizer::minimumVariance(diagonalCovariance);
        checkWeights(result.weights);
        require(result.converged, "Optimizer should converge on diagonal case");
        require(near(result.weights[0], 0.8, 1e-5),
                "Lower-variance asset should receive 80% weight");
        require(near(result.weights[1], 0.2, 1e-5),
                "Higher-variance asset should receive 20% weight");
        require(near(result.variance, 0.008, 1e-7),
                "Minimum variance should be 0.008");
        require(near(result.volatility, std::sqrt(0.008), 1e-7),
                "Volatility should equal square root of variance");

        // 2. Perfectly identical covariance rows allow a valid solution.
        const auto identical = ql::PortfolioOptimizer::minimumVariance({
            {0.04, 0.04},
            {0.04, 0.04}
        });
        checkWeights(identical.weights);
        require(near(identical.variance, 0.04),
                "Identical assets should preserve the same variance");

        // 3. Expected portfolio return is the weighted sum.
        require(near(
                    ql::PortfolioOptimizer::expectedReturn(
                        {0.8, 0.2}, {0.10, 0.04}),
                    0.088),
                "Expected return should be weighted sum");

        // 4. Zero covariance matrix is handled without iteration.
        const auto zero = ql::PortfolioOptimizer::minimumVariance({
            {0.0, 0.0},
            {0.0, 0.0}
        });
        require(zero.converged && near(zero.variance, 0.0),
                "Zero covariance matrix should converge immediately");
        checkWeights(zero.weights);

        // 5. Invalid optimizer arguments and matrices.
        requireThrows([] {
            (void)ql::PortfolioOptimizer::minimumVariance({});
        }, "Empty covariance matrix should throw");

        requireThrows([] {
            (void)ql::PortfolioOptimizer::minimumVariance({
                {0.01, 0.0},
                {0.0}
            });
        }, "Non-square covariance matrix should throw");

        requireThrows([] {
            (void)ql::PortfolioOptimizer::minimumVariance({
                {0.01, 0.02},
                {0.03, 0.04}
            });
        }, "Asymmetric covariance matrix should throw");

        requireThrows([] {
            (void)ql::PortfolioOptimizer::minimumVariance({
                {1.0, 0.0},
                {0.0, -0.1}
            });
        }, "Non-PSD covariance matrix should throw");

        requireThrows([] {
            (void)ql::PortfolioOptimizer::minimumVariance(
                {{0.01}}, 0);
        }, "Zero maximum iterations should throw");

        requireThrows([] {
            (void)ql::PortfolioOptimizer::minimumVariance(
                {{0.01}}, 100, std::numeric_limits<double>::infinity());
        }, "Non-finite tolerance should throw");

        requireThrows([] {
            (void)ql::PortfolioOptimizer::expectedReturn(
                {0.5}, {0.1, 0.2});
        }, "Expected return dimension mismatch should throw");

        requireThrows([] {
            (void)ql::PortfolioOptimizer::expectedReturn(
                {0.5, std::numeric_limits<double>::quiet_NaN()}, {0.1, 0.2});
        }, "Non-finite weight should throw");

        std::cout << "All portfolio optimizer tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Portfolio optimizer test failure: "
                  << error.what() << '\n';
        return 1;
    }
}
