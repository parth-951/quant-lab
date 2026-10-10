
#include "parametric_es.hpp"

#include "portfolio_volatility.hpp"
#include "normal_distribution.hpp"
#include "portfolio_weight_validation.hpp"

#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <vector>

namespace ql {

double ParametricES::calculate(
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
    const double volatility =
        PortfolioVolatility::volatility(weights, covarianceMatrix);

    if (!std::isfinite(volatility) || volatility < 0.0) {
        throw std::invalid_argument(
            "Portfolio volatility must be finite and nonnegative.");
    }

    if (portfolioValue == 0.0 || volatility == 0.0) {
        return 0.0;
    }

    const double z =
        NormalDistribution::inverseCdf(confidenceLevel);

    const double density = NormalDistribution::pdf(z);

    const double result =
        portfolioValue
        * volatility
        * std::sqrt(static_cast<double>(horizonDays))
        * density
        / (1.0 - confidenceLevel);

    if (!std::isfinite(result)) {
        throw std::overflow_error(
            "Calculated ES is not finite.");
    }

    return result;
}

} // namespace ql
