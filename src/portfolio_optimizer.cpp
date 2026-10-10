#include "portfolio_optimizer.hpp"

#include "portfolio_volatility.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <vector>

namespace ql {
namespace {

using Matrix = CovarianceMatrix::Matrix;

void validateCovariance(const Matrix& covariance) {
    if (covariance.empty()) {
        throw std::invalid_argument(
            "Optimizer requires at least one asset");
    }

    const std::size_t n = covariance.size();
    double scale = 1.0;

    for (std::size_t i = 0; i < n; ++i) {
        if (covariance[i].size() != n) {
            throw std::invalid_argument(
                "Covariance matrix must be square");
        }

        for (double value : covariance[i]) {
            if (!std::isfinite(value)) {
                throw std::invalid_argument(
                    "Covariance matrix values must be finite");
            }
            scale = std::max(scale, std::abs(value));
        }
    }

    const double symmetryTolerance = 1e-10 * scale;
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = i + 1; j < n; ++j) {
            if (std::abs(covariance[i][j] - covariance[j][i]) >
                symmetryTolerance) {
                throw std::invalid_argument(
                    "Covariance matrix must be symmetric");
            }
        }
    }

    // Jacobi eigenvalue iteration: a symmetric covariance matrix must be
    // positive semidefinite. This check avoids optimizing an invalid matrix.
    Matrix diagonalized = covariance;
    const std::size_t maxJacobiIterations = std::max<std::size_t>(1, 50 * n * n);
    const double eigenTolerance = 1e-12 * scale;

    for (std::size_t iteration = 0;
         iteration < maxJacobiIterations;
         ++iteration) {
        std::size_t p = 0;
        std::size_t q = 0;
        double largestOffDiagonal = 0.0;

        for (std::size_t i = 0; i < n; ++i) {
            for (std::size_t j = i + 1; j < n; ++j) {
                const double magnitude = std::abs(diagonalized[i][j]);
                if (magnitude > largestOffDiagonal) {
                    largestOffDiagonal = magnitude;
                    p = i;
                    q = j;
                }
            }
        }

        if (largestOffDiagonal <= eigenTolerance) {
            break;
        }

        const double app = diagonalized[p][p];
        const double aqq = diagonalized[q][q];
        const double apq = diagonalized[p][q];
        const double angle = 0.5 * std::atan2(2.0 * apq, aqq - app);
        const double c = std::cos(angle);
        const double s = std::sin(angle);

        for (std::size_t k = 0; k < n; ++k) {
            if (k == p || k == q) {
                continue;
            }
            const double akp = diagonalized[k][p];
            const double akq = diagonalized[k][q];
            diagonalized[k][p] = c * akp - s * akq;
            diagonalized[p][k] = diagonalized[k][p];
            diagonalized[k][q] = s * akp + c * akq;
            diagonalized[q][k] = diagonalized[k][q];
        }

        diagonalized[p][p] =
            c * c * app - 2.0 * s * c * apq + s * s * aqq;
        diagonalized[q][q] =
            s * s * app + 2.0 * s * c * apq + c * c * aqq;
        diagonalized[p][q] = 0.0;
        diagonalized[q][p] = 0.0;
    }

    for (std::size_t i = 0; i < n; ++i) {
        if (diagonalized[i][i] < -1e-10 * scale) {
            throw std::invalid_argument(
                "Covariance matrix must be positive semidefinite");
        }
    }
}

std::vector<double> projectToSimplex(const std::vector<double>& values) {
    // Euclidean projection onto {w: w_i >= 0, sum(w_i) = 1}.
    std::vector<double> sorted = values;
    std::sort(sorted.begin(), sorted.end(), std::greater<double>());

    double prefixSum = 0.0;
    double theta = 0.0;
    std::size_t rho = 0;

    for (std::size_t i = 0; i < sorted.size(); ++i) {
        prefixSum += sorted[i];
        const double candidate = (prefixSum - 1.0) /
                                 static_cast<double>(i + 1);
        if (sorted[i] - candidate > 0.0) {
            rho = i + 1;
            theta = candidate;
        }
    }

    if (rho == 0) {
        throw std::runtime_error("Could not project weights onto simplex");
    }

    std::vector<double> projected(values.size());
    for (std::size_t i = 0; i < values.size(); ++i) {
        projected[i] = std::max(0.0, values[i] - theta);
    }
    return projected;
}

std::vector<double> multiply(const Matrix& matrix,
                             const std::vector<double>& vector) {
    std::vector<double> result(vector.size(), 0.0);
    for (std::size_t i = 0; i < matrix.size(); ++i) {
        long double sum = 0.0L;
        for (std::size_t j = 0; j < vector.size(); ++j) {
            sum += static_cast<long double>(matrix[i][j]) * vector[j];
        }
        result[i] = static_cast<double>(sum);
        if (!std::isfinite(result[i])) {
            throw std::overflow_error(
                "Covariance multiplication produced a non-finite value");
        }
    }
    return result;
}

double maxAbsoluteRowSum(const Matrix& matrix) {
    double bound = 0.0;
    for (const auto& row : matrix) {
        double rowSum = 0.0;
        for (double value : row) {
            rowSum += std::abs(value);
        }
        bound = std::max(bound, rowSum);
    }
    return bound;
}

} // namespace

PortfolioOptimizationResult PortfolioOptimizer::minimumVariance(
    const CovarianceMatrix::Matrix& covarianceMatrix,
    std::size_t maxIterations,
    double tolerance) {
    validateCovariance(covarianceMatrix);

    if (maxIterations == 0) {
        throw std::invalid_argument(
            "Maximum iterations must be greater than zero");
    }
    if (!std::isfinite(tolerance) || tolerance <= 0.0) {
        throw std::invalid_argument(
            "Tolerance must be finite and greater than zero");
    }

    const std::size_t n = covarianceMatrix.size();
    std::vector<double> weights(n, 1.0 / static_cast<double>(n));
    const double lipschitzBound = maxAbsoluteRowSum(covarianceMatrix);

    PortfolioOptimizationResult result;
    if (lipschitzBound == 0.0) {
        result.weights = weights;
        result.variance = 0.0;
        result.volatility = 0.0;
        result.iterations = 0;
        result.converged = true;
        return result;
    }

    // For f(w) = w^T Sigma w, gradient is 2 Sigma w. The maximum absolute
    // row sum bounds the spectral norm of a symmetric matrix.
    const double stepSize = 1.0 / (2.0 * lipschitzBound);

    for (std::size_t iteration = 1; iteration <= maxIterations; ++iteration) {
        const std::vector<double> sigmaW =
            multiply(covarianceMatrix, weights);

        std::vector<double> candidate(n);
        for (std::size_t i = 0; i < n; ++i) {
            candidate[i] = weights[i] - stepSize * 2.0 * sigmaW[i];
        }

        candidate = projectToSimplex(candidate);

        double maxChange = 0.0;
        for (std::size_t i = 0; i < n; ++i) {
            maxChange = std::max(
                maxChange, std::abs(candidate[i] - weights[i]));
        }

        weights = std::move(candidate);
        result.iterations = iteration;

        if (maxChange <= tolerance) {
            result.converged = true;
            break;
        }
    }

    result.weights = weights;
    result.variance =
        PortfolioVolatility::variance(weights, covarianceMatrix);
    result.volatility = std::sqrt(result.variance);
    return result;
}

double PortfolioOptimizer::expectedReturn(
    const std::vector<double>& weights,
    const std::vector<double>& assetExpectedReturns) {
    if (weights.empty() || weights.size() != assetExpectedReturns.size()) {
        throw std::invalid_argument(
            "Weights and expected returns must have the same non-zero size");
    }

    long double result = 0.0L;
    for (std::size_t i = 0; i < weights.size(); ++i) {
        if (!std::isfinite(weights[i]) ||
            !std::isfinite(assetExpectedReturns[i])) {
            throw std::invalid_argument(
                "Weights and expected returns must be finite");
        }
        result += static_cast<long double>(weights[i]) *
                  static_cast<long double>(assetExpectedReturns[i]);
    }

    const double expected = static_cast<double>(result);
    if (!std::isfinite(expected)) {
        throw std::overflow_error(
            "Expected return calculation produced a non-finite result");
    }
    return expected;
}

} // namespace ql
