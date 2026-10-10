#pragma once

#include "covariance_matrix.hpp"

#include <cstddef>
#include <vector>

namespace ql {

struct PortfolioOptimizationResult {
    std::vector<double> weights;
    double variance = 0.0;
    double volatility = 0.0;
    std::size_t iterations = 0;
    bool converged = false;
};

class PortfolioOptimizer {
public:
    // Finds a long-only minimum-variance portfolio.
    // Constraints: each weight is in [0, 1] and weights sum to 1.
    // covarianceMatrix must be a finite, symmetric, positive-semidefinite
    // sample covariance matrix. Initial weights are equal-weighted.
    [[nodiscard]] static PortfolioOptimizationResult minimumVariance(
        const CovarianceMatrix::Matrix& covarianceMatrix,
        std::size_t maxIterations = 200000,
        double tolerance = 1e-12);

    // Computes weighted expected portfolio return.
    [[nodiscard]] static double expectedReturn(
        const std::vector<double>& weights,
        const std::vector<double>& assetExpectedReturns);
};

} // namespace ql
