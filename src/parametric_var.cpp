#include "parametric_var.hpp"

#include "portfolio_volatility.hpp"

#include <cmath>
#include <limits>
#include <stdexcept>

namespace ql {
namespace {

// Acklam's rational approximation to the inverse standard normal CDF.
// Input probability must be strictly between zero and one.
double inverseNormalCdf(double p) {
    constexpr double a1 = -3.969683028665376e+01;
    constexpr double a2 =  2.209460984245205e+02;
    constexpr double a3 = -2.759285104469687e+02;
    constexpr double a4 =  1.383577518672690e+02;
    constexpr double a5 = -3.066479806614716e+01;
    constexpr double a6 =  2.506628277459239e+00;

    constexpr double b1 = -5.447609879822406e+01;
    constexpr double b2 =  1.615858368580409e+02;
    constexpr double b3 = -1.556989798598866e+02;
    constexpr double b4 =  6.680131188771972e+01;
    constexpr double b5 = -1.328068155288572e+01;

    constexpr double c1 = -7.784894002430293e-03;
    constexpr double c2 = -3.223964580411365e-01;
    constexpr double c3 = -2.400758277161838e+00;
    constexpr double c4 = -2.549732539343734e+00;
    constexpr double c5 =  4.374664141464968e+00;
    constexpr double c6 =  2.938163982698783e+00;

    constexpr double d1 =  7.784695709041462e-03;
    constexpr double d2 =  3.224671290700398e-01;
    constexpr double d3 =  2.445134137142996e+00;
    constexpr double d4 =  3.754408661907416e+00;

    constexpr double low = 0.02425;
    constexpr double high = 1.0 - low;

    if (p < low) {
        const double q = std::sqrt(-2.0 * std::log(p));
        return (((((c1 * q + c2) * q + c3) * q + c4) * q + c5) * q + c6) /
               ((((d1 * q + d2) * q + d3) * q + d4) * q + 1.0);
    }

    if (p > high) {
        const double q = std::sqrt(-2.0 * std::log(1.0 - p));
        return -(((((c1 * q + c2) * q + c3) * q + c4) * q + c5) * q + c6) /
                ((((d1 * q + d2) * q + d3) * q + d4) * q + 1.0);
    }

    const double q = p - 0.5;
    const double r = q * q;

    return (((((a1 * r + a2) * r + a3) * r + a4) * r + a5) * r + a6) * q /
           (((((b1 * r + b2) * r + b3) * r + b4) * r + b5) * r + 1.0);
}

} // namespace

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
        throw std::invalid_argument("Horizon must be at least one day.");
    }

    if (weights.empty()) {
        throw std::invalid_argument("Portfolio weights must not be empty.");
    }

    for (double weight : weights) {
        if (!std::isfinite(weight)) {
            throw std::invalid_argument(
                "Portfolio weights must all be finite.");
        }
    }

    // Validate covariance dimensions and values even for a zero-value
    // portfolio, by routing through the existing portfolio-risk API.
    const double dailyVolatility =
        PortfolioVolatility::volatility(weights, covarianceMatrix);

    if (!std::isfinite(dailyVolatility) || dailyVolatility < 0.0) {
        throw std::invalid_argument(
            "Portfolio volatility must be finite and nonnegative.");
    }

    if (portfolioValue == 0.0 || dailyVolatility == 0.0) {
        return 0.0;
    }

    const double zScore = inverseNormalCdf(confidenceLevel);
    const double horizonScale = std::sqrt(static_cast<double>(horizonDays));

    const double var =
        portfolioValue * zScore * dailyVolatility * horizonScale;

    if (!std::isfinite(var)) {
        throw std::overflow_error("Calculated VaR is not finite.");
    }

    return var;
}

} // namespace ql
