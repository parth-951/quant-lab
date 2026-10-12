
#include "parametric_var.hpp"

#include "portfolio_volatility.hpp"
#include "normal_distribution.hpp"
#include "portfolio_weight_validation.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <vector>

namespace ql {

double ParametricVaR::calculate(
    double portfolioValue,
    const std::vector<double>& weights,
    const CovarianceMatrix::Matrix& covarianceMatrix,
    double confidenceLevel,
    std::size_t horizonDays) {

    return calculate(
        portfolioValue,
        weights,
        covarianceMatrix,
        confidenceLevel,
        horizonDays,
        0.0);
}

double ParametricVaR::calculate(
    double portfolioValue,
    const std::vector<double>& weights,
    const CovarianceMatrix::Matrix& covarianceMatrix,
    double confidenceLevel,
    std::size_t horizonDays,
    double expectedReturn) {

    if (!std::isfinite(portfolioValue) || portfolioValue < 0.0) {
        throw std::invalid_argument(
            "Portfolio value must be finite and nonnegative.");
    }

    if (!std::isfinite(confidenceLevel) ||
        confidenceLevel <= 0.5 ||
        confidenceLevel >= 1.0) {
        throw std::invalid_argument(
            "Confidence level must be greater than 0.5 and less than 1.0.");
    }

    if (horizonDays == 0) {
        throw std::invalid_argument(
            "Horizon must be at least one day.");
    }

    if (!std::isfinite(expectedReturn)) {
        throw std::invalid_argument(
            "Expected return must be finite.");
    }

    validatePortfolioWeights(weights);

    const double dailyVolatility =
        PortfolioVolatility::volatility(weights, covarianceMatrix);

    if (!std::isfinite(dailyVolatility) || dailyVolatility < 0.0) {
        throw std::invalid_argument(
            "Portfolio volatility must be finite and nonnegative.");
    }

    if (portfolioValue == 0.0) {
        return 0.0;
    }

    const double zScore =
        NormalDistribution::inverseCdf(confidenceLevel);

    const double horizon =
        static_cast<double>(horizonDays);

    const double horizonScale = std::sqrt(horizon);

    const double expectedHorizonReturn =
        expectedReturn * horizon;

    const double horizonVolatility =
        dailyVolatility * horizonScale;

    const double lossFraction =
        zScore * horizonVolatility - expectedHorizonReturn;

    const double var = portfolioValue * lossFraction;

    if (!std::isfinite(expectedHorizonReturn) ||
        !std::isfinite(horizonVolatility) ||
        !std::isfinite(lossFraction) ||
        !std::isfinite(var)) {
        throw std::overflow_error(
            "Calculated VaR is not finite.");
    }

    // VaR is reported as a nonnegative loss threshold.
    return std::max(0.0, var);
}

} // namespace ql
