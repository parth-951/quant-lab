#include "efficient_frontier.hpp"

#include <algorithm>
#include <cmath>
#include <exception>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

int failures = 0;

void require(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    } else {
        std::cout << "PASS: " << message << '\n';
    }
}

bool near(double a, double b, double tolerance = 1e-6) {
    return std::abs(a - b) <= tolerance;
}

template <typename Function>
void requireThrows(Function function, const std::string& message) {
    try {
        function();
        require(false, message);
    } catch (const std::exception&) {
        require(true, message);
    }
}

void checkWeights(const std::vector<double>& weights) {
    double sum = 0.0;
    for (double weight : weights) {
        require(weight >= -1e-9 && weight <= 1.0 + 1e-9,
                "Weights remain long-only and bounded");
        sum += weight;
    }
    require(near(sum, 1.0, 1e-8), "Weights sum to one");
}

} // namespace

int main() {
    const ql::CovarianceMatrix::Matrix covariance{
        {0.01, 0.0},
        {0.0, 0.04}
    };
    const std::vector<double> returns{0.10, 0.04};

    // The global minimum variance allocation is 80% / 20%, with expected
    // return 8.8% and variance 0.008.
    const auto baseline =
        ql::EfficientFrontier::forTargetReturn(covariance, returns, 0.08);
    checkWeights(baseline.weights);
    require(baseline.converged, "Baseline solution converges");
    require(near(baseline.weights[0], 0.8, 1e-5),
            "Low target returns the minimum-variance portfolio");
    require(near(baseline.expectedReturn, 0.088, 1e-5),
            "Baseline expected return is correct");
    require(near(baseline.variance, 0.008, 1e-6),
            "Baseline variance is correct");

    // For target 9.4%, the constraint is binding: 90% * 10% +
    // 10% * 4% = 9.4%. Its variance is 0.9^2*0.01 + 0.1^2*0.04.
    const auto target =
        ql::EfficientFrontier::forTargetReturn(covariance, returns, 0.094);
    checkWeights(target.weights);
    require(target.converged, "Target-return solution converges");
    require(near(target.weights[0], 0.9, 1e-5),
            "Target return gives the analytical 90% allocation");
    require(near(target.expectedReturn, 0.094, 1e-6),
            "Target expected return is achieved");
    require(near(target.variance, 0.0085, 1e-6),
            "Target portfolio variance matches analytical value");

    const auto maximum =
        ql::EfficientFrontier::forTargetReturn(covariance, returns, 0.10);
    checkWeights(maximum.weights);
    require(near(maximum.weights[0], 1.0, 1e-8),
            "Maximum target invests in the highest-return asset");
    require(near(maximum.expectedReturn, 0.10, 1e-8),
            "Maximum expected return is achieved");

    requireThrows([&] {
        (void)ql::EfficientFrontier::forTargetReturn(
            covariance, returns, 0.101);
    }, "Infeasible target return throws");

    requireThrows([&] {
        (void)ql::EfficientFrontier::forTargetReturn(
            covariance, {0.1}, 0.08);
    }, "Return dimension mismatch throws");

    requireThrows([&] {
        (void)ql::EfficientFrontier::forTargetReturn(
            covariance, {0.1, std::numeric_limits<double>::quiet_NaN()}, 0.08);
    }, "Non-finite asset return throws");

    requireThrows([&] {
        (void)ql::EfficientFrontier::forTargetReturn(
            covariance, returns, std::numeric_limits<double>::infinity());
    }, "Non-finite target return throws");

    requireThrows([&] {
        (void)ql::EfficientFrontier::generate(covariance, returns, 0);
    }, "Zero frontier points throws");

    const auto frontier =
        ql::EfficientFrontier::generate(covariance, returns, 6);
    require(frontier.size() == 6, "Frontier contains requested point count");

    bool returnsNondecreasing = true;
    bool riskNondecreasing = true;
    for (std::size_t i = 0; i < frontier.size(); ++i) {
        checkWeights(frontier[i].weights);
        require(frontier[i].expectedReturn + 1e-6 >=
                    frontier[i].targetReturn,
                "Each frontier point meets its target return");
        if (i > 0) {
            if (frontier[i].expectedReturn + 1e-6 <
                frontier[i - 1].expectedReturn) {
                returnsNondecreasing = false;
            }
            if (frontier[i].volatility + 1e-6 <
                frontier[i - 1].volatility) {
                riskNondecreasing = false;
            }
        }
    }
    require(returnsNondecreasing, "Frontier expected returns are nondecreasing");
    require(riskNondecreasing, "Frontier volatility is nondecreasing");

    std::cout << (failures == 0
        ? "All efficient frontier tests passed.\n"
        : "Efficient frontier tests failed.\n");
    return failures == 0 ? 0 : 1;
}
