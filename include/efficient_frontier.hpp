#pragma once

#include "covariance_matrix.hpp"

#include <cstddef>
#include <vector>

namespace ql {

struct EfficientFrontierPoint {
    double targetReturn = 0.0;
    double expectedReturn = 0.0;
    double variance = 0.0;
    double volatility = 0.0;
    std::vector<double> weights;
    std::size_t iterations = 0;
    bool converged = false;
};

class EfficientFrontier {
public:
    // Finds the minimum-variance long-only portfolio whose expected return
    // is at least targetReturn. Weights lie in [0, 1] and sum to one.
    //
    // The covariance matrix must be finite, symmetric, and positive
    // semidefinite. Asset expected returns must use the same horizon as the
    // covariance matrix. A target above the highest asset expected return
    // is infeasible and throws std::invalid_argument.
    [[nodiscard]] static EfficientFrontierPoint forTargetReturn(
        const CovarianceMatrix::Matrix& covarianceMatrix,
        const std::vector<double>& assetExpectedReturns,
        double targetReturn,
        std::size_t maxIterations = 200000,
        double tolerance = 1e-10);

    // Generates the efficient branch from the global minimum-variance
    // portfolio's expected return through the highest individual asset
    // expected return. Returns one point if that interval is degenerate.
    [[nodiscard]] static std::vector<EfficientFrontierPoint> generate(
        const CovarianceMatrix::Matrix& covarianceMatrix,
        const std::vector<double>& assetExpectedReturns,
        std::size_t points = 20,
        std::size_t maxIterations = 200000,
        double tolerance = 1e-10);
};

} // namespace ql
