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
// - Square-root-of-time scaling is used for multi-day horizons.
// - Weights are portfolio fractions and must sum to 1.0.
// - Short positions are permitted if the weights still sum to 1.0.
// - Expected return is assumed to be zero.
//
// Returns a nonnegative monetary loss threshold in the same currency
// as portfolioValue.
//
// Example:
// portfolioValue = 1,000,000
// weights = {0.6, 0.4}
// The monetary VaR is calculated from the portfolio's return volatility.
class ParametricVaR {
public:
    [[nodiscard]] static double calculate(
        double portfolioValue,
        const std::vector<double>& weights,
        const CovarianceMatrix::Matrix& covarianceMatrix,
        double confidenceLevel,
        std::size_t horizonDays = 1);
};

} // namespace ql