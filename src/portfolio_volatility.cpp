
#include "portfolio_volatility.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <vector>

namespace ql {
namespace {
void validatePositiveSemidefinite(
    const CovarianceMatrix::Matrix& covarianceMatrix);
void validateInputs(
    const std::vector<double>& weights,
    const CovarianceMatrix::Matrix& covarianceMatrix) {
    if (weights.empty()) {
        throw std::invalid_argument(
            "Portfolio volatility requires at least one weight");
    }

    const std::size_t n = weights.size();

    if (covarianceMatrix.size() != n) {
        throw std::invalid_argument(
            "Covariance matrix dimensions must match the number of weights");
    }

    for (double weight : weights) {
        if (!std::isfinite(weight)) {
            throw std::invalid_argument(
                "Portfolio weights must all be finite");
        }
    }

    // Pass 1: validate every row before indexing any other row.
    // This prevents out-of-bounds access for malformed matrices.
    for (const auto& row : covarianceMatrix) {
        if (row.size() != n) {
            throw std::invalid_argument(
                "Covariance matrix must be square");
        }

        for (double value : row) {
            if (!std::isfinite(value)) {
                throw std::invalid_argument(
                    "Covariance matrix values must all be finite");
            }
        }
    }

    // Pass 2: check symmetry only after all dimensions are validated.
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = i + 1; j < n; ++j) {
            const double a = covarianceMatrix[i][j];
            const double b = covarianceMatrix[j][i];

            const double tolerance =
                1e-10 * std::max({1.0, std::abs(a), std::abs(b)});

            if (std::abs(a - b) > tolerance) {
                throw std::invalid_argument(
                    "Covariance matrix must be symmetric");
            }
        }
    }
    validatePositiveSemidefinite(covarianceMatrix);
}

void validatePositiveSemidefinite(
    const CovarianceMatrix::Matrix& covarianceMatrix) {
    const std::size_t n = covarianceMatrix.size();

    // A zero matrix is positive semidefinite.
    double matrixScale = 0.0;
    for (const auto& row : covarianceMatrix) {
        for (double value : row) {
            matrixScale = std::max(matrixScale, std::abs(value));
        }
    }

    if (matrixScale == 0.0) {
        return;
    }

    // Work with a scaled copy to improve numerical stability.
    // Average symmetric entries to account for tiny accepted asymmetry.
    CovarianceMatrix::Matrix matrix(
        n, std::vector<double>(n, 0.0));

    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = 0; j < n; ++j) {
            matrix[i][j] =
                0.5 * (covarianceMatrix[i][j] / matrixScale) +
                0.5 * (covarianceMatrix[j][i] / matrixScale);
        }
    }

    // Jacobi eigenvalue algorithm: repeatedly eliminate the largest
    // off-diagonal element using a plane rotation.
    const double convergenceTolerance =
        100.0 * std::numeric_limits<double>::epsilon();

    const std::size_t maxIterations = 100 * n * n;
    bool converged = (n == 1);

    for (std::size_t iteration = 0;
         iteration < maxIterations && !converged;
         ++iteration) {
        std::size_t p = 0;
        std::size_t q = 1;
        double largestOffDiagonal = 0.0;

        for (std::size_t i = 0; i < n; ++i) {
            for (std::size_t j = i + 1; j < n; ++j) {
                const double magnitude = std::abs(matrix[i][j]);

                if (magnitude > largestOffDiagonal) {
                    largestOffDiagonal = magnitude;
                    p = i;
                    q = j;
                }
            }
        }

        if (largestOffDiagonal <= convergenceTolerance) {
            converged = true;
            break;
        }

        const double app = matrix[p][p];
        const double aqq = matrix[q][q];
        const double apq = matrix[p][q];

        const double tau = (aqq - app) / (2.0 * apq);
        const double t =
            std::copysign(1.0, tau) /
            (std::abs(tau) + std::hypot(1.0, tau));

        const double c = 1.0 / std::hypot(1.0, t);
        const double s = t * c;

        matrix[p][p] = app - t * apq;
        matrix[q][q] = aqq + t * apq;
        matrix[p][q] = 0.0;
        matrix[q][p] = 0.0;

        for (std::size_t k = 0; k < n; ++k) {
            if (k == p || k == q) {
                continue;
            }

            const double akp = matrix[k][p];
            const double akq = matrix[k][q];

            matrix[k][p] = c * akp - s * akq;
            matrix[p][k] = matrix[k][p];

            matrix[k][q] = s * akp + c * akq;
            matrix[q][k] = matrix[k][q];
        }
    }

    if (!converged) {
        throw std::runtime_error(
            "Covariance matrix PSD validation did not converge");
    }

    // The matrix was scaled so its largest absolute entry is 1.
    // Allow a small relative negative eigenvalue from round-off.
    const double psdTolerance =
        1e-10 * static_cast<double>(n);

    for (std::size_t i = 0; i < n; ++i) {
        if (matrix[i][i] < -psdTolerance) {
            throw std::domain_error(
                "Covariance matrix must be positive semidefinite");
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
