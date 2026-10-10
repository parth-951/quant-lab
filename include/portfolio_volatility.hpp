#pragma once

#include "covariance_matrix.hpp"

#include <vector>

namespace ql {

// Computes portfolio variance and volatility from portfolio weights and a
// covariance matrix of aligned asset returns. Weights need not sum to one;
// they may represent any consistent exposure convention.
class PortfolioVolatility {
public:
    [[nodiscard]] static double variance(
        const std::vector<double>& weights,
        const CovarianceMatrix::Matrix& covarianceMatrix);

    [[nodiscard]] static double volatility(
        const std::vector<double>& weights,
        const CovarianceMatrix::Matrix& covarianceMatrix);
};

} // namespace ql
