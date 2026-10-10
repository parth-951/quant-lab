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
// - Weights use the same exposure convention as PortfolioVolatility.
//
// Returns a nonnegative monetary loss threshold in the same currency as
// portfolioValue. For example, 95% VaR is the loss threshold exceeded with
// approximately 5% probability under the model.
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
