#pragma once

#include "covariance_matrix.hpp"

#include <cstddef>
#include <vector>

namespace ql {

// Parametric Expected Shortfall (ES), also called Conditional VaR.
//
// Assumptions:
// - Asset returns are approximately normally distributed.
// - The covariance matrix represents one-day asset returns.
// - Expected return is assumed to be zero.
// - Square-root-of-time scaling is used for multi-day horizons.
// - Weights are portfolio fractions and must sum to 1.0.
//
// Returns a nonnegative monetary loss in the same currency as portfolioValue.
class ParametricES {
public:
    [[nodiscard]] static double calculate(
        double portfolioValue,
        const std::vector<double>& weights,
        const CovarianceMatrix::Matrix& covarianceMatrix,
        double confidenceLevel,
        std::size_t horizonDays = 1);
};

} // namespace ql