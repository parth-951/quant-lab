
#pragma once

#include "covariance_matrix.hpp"

#include <cstddef>
#include <vector>

namespace ql {

// Parametric (variance-covariance) Value at Risk.
//
// Assumptions:
// - Asset returns are approximately normally distributed.
// - The covariance matrix represents one-day asset returns.
// - Expected return is a daily arithmetic portfolio return.
// - Volatility scales with sqrt(horizonDays).
// - Expected return scales linearly with horizonDays.
// - Weights are portfolio fractions and must sum to 1.0.
// - Short positions are permitted if weights sum to 1.0.
//
// Returns a nonnegative monetary loss threshold.
class ParametricVaR {
public:
    // Backward-compatible overload: assumes zero expected return.
    [[nodiscard]] static double calculate(
        double portfolioValue,
        const std::vector<double>& weights,
        const CovarianceMatrix::Matrix& covarianceMatrix,
        double confidenceLevel,
        std::size_t horizonDays = 1);

    // expectedReturn is the expected daily portfolio return.
    [[nodiscard]] static double calculate(
        double portfolioValue,
        const std::vector<double>& weights,
        const CovarianceMatrix::Matrix& covarianceMatrix,
        double confidenceLevel,
        std::size_t horizonDays,
        double expectedReturn);
};

} // namespace ql
