#include "portfolio_volatility.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <limits>
#include <vector>

namespace ql {
namespace {

void validateInputs(
    const std::vector<double>& weights,
    const CovarianceMatrix::Matrix& covarianceMatrix) {
    if (weights.empty()) {
        throw std::invalid_argument(
            "Portfolio volatility requires at least one weight");
    }

    if (covarianceMatrix.size() != weights.size()) {
        throw std::invalid_argument(
            "Covariance matrix dimensions must match the number of weights");
    }

    for (double weight : weights) {
        if (!std::isfinite(weight)) {
            throw std::invalid_argument(
                "Portfolio weights must all be finite");
        }
    }

    const std::size_t n = weights.size();
    for (std::size_t i = 0; i < n; ++i) {
        if (covarianceMatrix[i].size() != n) {
            throw std::invalid_argument(
                "Covariance matrix must be square");
        }

        for (std::size_t j = 0; j < n; ++j) {
            if (!std::isfinite(covarianceMatrix[i][j])) {
                throw std::invalid_argument(
                    "Covariance matrix values must all be finite");
            }

            const double a = covarianceMatrix[i][j];
            const double b = covarianceMatrix[j].size() == n
                ? covarianceMatrix[j][i]
                : a;
            const double tolerance =
                1e-10 * std::max({1.0, std::abs(a), std::abs(b)});

            if (std::abs(a - b) > tolerance) {
                throw std::invalid_argument(
                    "Covariance matrix must be symmetric");
            }
        }
    }
}

} // namespace

double PortfolioVolatility::variance(
    const std::vector<double>& weights,
    const CovarianceMatrix::Matrix& covarianceMatrix) {
    validateInputs(weights, covarianceMatrix);

    const std::size_t n = weights.size();
    long double result = 0.0L;

    // w^T * Sigma * w
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = 0; j < n; ++j) {
            result += static_cast<long double>(weights[i]) *
                      static_cast<long double>(covarianceMatrix[i][j]) *
                      static_cast<long double>(weights[j]);
        }
    }

    double portfolioVariance = static_cast<double>(result);
    if (!std::isfinite(portfolioVariance)) {
        throw std::overflow_error(
            "Portfolio variance calculation produced a non-finite result");
    }

    // A valid covariance matrix cannot produce negative variance. Permit tiny
    // negative round-off, but reject materially negative results.
    double scale = 1.0;
    for (const auto& row : covarianceMatrix) {
        for (double value : row) {
            scale = std::max(scale, std::abs(value));
        }
    }
    for (double weight : weights) {
        scale *= std::max(1.0, std::abs(weight));
        if (!std::isfinite(scale)) {
            scale = std::numeric_limits<double>::max();
            break;
        }
    }
    const double tolerance = 1e-12 * scale;

    if (portfolioVariance < -tolerance) {
        throw std::domain_error(
            "Portfolio variance is negative; covariance matrix may not be positive semidefinite");
    }

    if (portfolioVariance < 0.0) {
        portfolioVariance = 0.0;
    }

    return portfolioVariance;
}

double PortfolioVolatility::volatility(
    const std::vector<double>& weights,
    const CovarianceMatrix::Matrix& covarianceMatrix) {
    return std::sqrt(variance(weights, covarianceMatrix));
}

} // namespace ql
