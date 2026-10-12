
#include "parametric_es.hpp"

#include "portfolio_volatility.hpp"
#include "normal_distribution.hpp"
#include "portfolio_weight_validation.hpp"

#include <algorithm>
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

    return calculate(
        portfolioValue,
        weights,
        covarianceMatrix,
        confidenceLevel,
        horizonDays,
        0.0);
}

double ParametricES::calculate(
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

    const double z =
        NormalDistribution::inverseCdf(confidenceLevel);

    const double density = NormalDistribution::pdf(z);

    const double horizon =
        static_cast<double>(horizonDays);

    const double expectedHorizonReturn =
        expectedReturn * horizon;

    const double horizonVolatility =
        dailyVolatility * std::sqrt(horizon);

    const double tailLossFraction =
        horizonVolatility * density / (1.0 - confidenceLevel)
        - expectedHorizonReturn;

    const double result = portfolioValue * tailLossFraction;

    if (!std::isfinite(expectedHorizonReturn) ||
        !std::isfinite(horizonVolatility) ||
        !std::isfinite(tailLossFraction) ||
        !std::isfinite(result)) {
        throw std::overflow_error(
            "Calculated ES is not finite.");
    }

    // ES is reported as a nonnegative expected loss.
    return std::max(0.0, result);
}

} // namespace ql
