
#include "parametric_var.hpp"

#include "portfolio_volatility.hpp"
#include "normal_distribution.hpp"
#include "portfolio_weight_validation.hpp"

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

    validatePortfolioWeights(weights);

    // Also validates covariance dimensions, finiteness, and symmetry.
    const double dailyVolatility =
        PortfolioVolatility::volatility(weights, covarianceMatrix);

    if (!std::isfinite(dailyVolatility) || dailyVolatility < 0.0) {
        throw std::invalid_argument(
            "Portfolio volatility must be finite and nonnegative.");
    }

    if (portfolioValue == 0.0 || dailyVolatility == 0.0) {
        return 0.0;
    }

    const double zScore =
        NormalDistribution::inverseCdf(confidenceLevel);

    const double horizonScale =
        std::sqrt(static_cast<double>(horizonDays));

    const double var =
        portfolioValue * zScore * dailyVolatility * horizonScale;

    if (!std::isfinite(var)) {
        throw std::overflow_error(
            "Calculated VaR is not finite.");
    }

    return var;
}

} // namespace ql
