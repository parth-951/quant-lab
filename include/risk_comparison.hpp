#pragma once

#include "covariance_matrix.hpp"

#include <cstddef>
#include <vector>

namespace ql {

struct RiskComparisonResult {
    double historicalVaR = 0.0;
    double historicalES = 0.0;
    double parametricVaR = 0.0;
    double parametricES = 0.0;
    double portfolioValue = 0.0;
    double confidenceLevel = 0.0;
    std::size_t horizonDays = 1;
};

class RiskComparison {
public:
    // portfolioReturns are one-day simple returns:
    // 0.01 means +1%, and -0.02 means -2%.
    //
    // Historical estimates use rolling, compounded returns from the supplied
    // history. Parametric estimates use the supplied one-day covariance matrix
    // and square-root-of-time scaling.
    //
    // portfolioValue is the current total portfolio value.
    // weights are portfolio fractions and must sum to 1.0.
    // Signed weights are allowed for long/short portfolios.
    //
    // Results are nonnegative monetary loss estimates in the same currency
    // as portfolioValue. Parametric estimates assume zero expected return.
    [[nodiscard]] static RiskComparisonResult calculate(
        const std::vector<double>& portfolioReturns,
        double portfolioValue,
        const std::vector<double>& weights,
        const CovarianceMatrix::Matrix& covarianceMatrix,
        double confidenceLevel,
        std::size_t horizonDays = 1);
};

} // namespace ql