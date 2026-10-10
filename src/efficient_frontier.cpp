#include "efficient_frontier.hpp"

#include "portfolio_optimizer.hpp"
#include "portfolio_volatility.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

namespace ql {
namespace {

using Matrix = CovarianceMatrix::Matrix;

void validateReturns(const std::vector<double>& returns, std::size_t n) {
    if (returns.size() != n || returns.empty()) {
        throw std::invalid_argument(
            "Expected returns must match the non-zero asset count");
    }
    for (double value : returns) {
        if (!std::isfinite(value)) {
            throw std::invalid_argument(
                "Asset expected returns must all be finite");
        }
    }
}

std::vector<double> projectToSimplex(const std::vector<double>& values) {
    std::vector<double> sorted = values;
    std::sort(sorted.begin(), sorted.end(), std::greater<double>());

    double prefixSum = 0.0;
    double theta = 0.0;
    std::size_t rho = 0;

    for (std::size_t i = 0; i < sorted.size(); ++i) {
        prefixSum += sorted[i];
        const double candidate =
            (prefixSum - 1.0) / static_cast<double>(i + 1);
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

double weightedReturn(
    const std::vector<double>& weights,
    const std::vector<double>& returns) {
    return PortfolioOptimizer::expectedReturn(weights, returns);
}

// Euclidean projection onto the intersection of the probability simplex
// and {w: expectedReturn(w) >= target}. The inequality's Lagrange multiplier
// shifts each coordinate by lambda * expectedReturn before simplex projection.
std::vector<double> projectToFeasible(
    const std::vector<double>& values,
    const std::vector<double>& returns,
    double target) {

    std::vector<double> projected = projectToSimplex(values);
    if (weightedReturn(projected, returns) >= target) {
        return projected;
    }

    double low = 0.0;
    double high = 1.0;
    auto shiftedProjection = [&](double lambda) {
        std::vector<double> shifted(values.size());
        for (std::size_t i = 0; i < values.size(); ++i) {
            shifted[i] = values[i] + lambda * returns[i];
            if (!std::isfinite(shifted[i])) {
                throw std::overflow_error(
                    "Projection overflowed while enforcing target return");
            }
        }
        return projectToSimplex(shifted);
    };

    // Find a multiplier whose projected portfolio meets the target.
    for (int expansion = 0; expansion < 1024; ++expansion) {
        projected = shiftedProjection(high);
        if (weightedReturn(projected, returns) >= target) {
            break;
        }
        high *= 2.0;
        if (!std::isfinite(high)) {
            throw std::overflow_error(
                "Could not find a feasible target-return projection");
        }
    }

    if (weightedReturn(projected, returns) < target) {
        throw std::invalid_argument("Target return is infeasible");
    }

    // Bisection converges to the boundary of the half-space/simplex
    // intersection; keep the feasible (upper-multiplier) endpoint.
    for (int iteration = 0; iteration < 100; ++iteration) {
        const double mid = low + (high - low) * 0.5;
        std::vector<double> candidate = shiftedProjection(mid);
        if (weightedReturn(candidate, returns) >= target) {
            high = mid;
            projected = std::move(candidate);
        } else {
            low = mid;
        }
    }

    // Recompute at the feasible endpoint to avoid returning a stale value.
    projected = shiftedProjection(high);
    return projected;
}

double maxAbsoluteRowSum(const Matrix& matrix) {
    double bound = 0.0;
    for (const auto& row : matrix) {
        long double sum = 0.0L;
        for (double value : row) {
            sum += std::abs(value);
        }
        const double rowSum = static_cast<double>(sum);
        if (!std::isfinite(rowSum)) {
            throw std::overflow_error("Covariance row sum is not finite");
        }
        bound = std::max(bound, rowSum);
    }
    return bound;
}

std::vector<double> multiply(
    const Matrix& matrix,
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

} // namespace

EfficientFrontierPoint EfficientFrontier::forTargetReturn(
    const CovarianceMatrix::Matrix& covarianceMatrix,
    const std::vector<double>& assetExpectedReturns,
    double targetReturn,
    std::size_t maxIterations,
    double tolerance) {

    if (!std::isfinite(targetReturn)) {
        throw std::invalid_argument("Target return must be finite");
    }
    if (maxIterations == 0) {
        throw std::invalid_argument(
            "Maximum iterations must be greater than zero");
    }
    if (!std::isfinite(tolerance) || tolerance <= 0.0) {
        throw std::invalid_argument(
            "Tolerance must be finite and greater than zero");
    }

    // Reuse the optimizer's covariance validation and baseline solution.
    const PortfolioOptimizationResult baseline =
        PortfolioOptimizer::minimumVariance(covarianceMatrix);
    const std::size_t n = covarianceMatrix.size();
    validateReturns(assetExpectedReturns, n);

    const double maximumAssetReturn =
        *std::max_element(assetExpectedReturns.begin(),
                          assetExpectedReturns.end());

    const double feasibilityTolerance =
        1e-12 * std::max(1.0, std::abs(maximumAssetReturn));

    if (targetReturn > maximumAssetReturn + feasibilityTolerance) {
        throw std::invalid_argument(
            "Target return exceeds the maximum asset expected return");
    }

    EfficientFrontierPoint result;
    result.targetReturn = targetReturn;

    const double baselineReturn =
        weightedReturn(baseline.weights, assetExpectedReturns);

    // The global minimum-variance portfolio is also optimal for every target
    // no greater than its expected return.
    if (targetReturn <= baselineReturn) {
        result.weights = baseline.weights;
        result.expectedReturn = baselineReturn;
        result.variance = baseline.variance;
        result.volatility = baseline.volatility;
        result.iterations = baseline.iterations;
        result.converged = baseline.converged;
        return result;
    }

    // At the maximum attainable return, only assets with the maximum return
    // can have positive weight. Solve the reduced covariance problem directly.
    if (targetReturn >= maximumAssetReturn) {
        std::vector<std::size_t> active;
        for (std::size_t i = 0; i < n; ++i) {
            if (assetExpectedReturns[i] == maximumAssetReturn) {
                active.push_back(i);
            }
        }

        Matrix reduced(active.size(), std::vector<double>(active.size()));
        for (std::size_t i = 0; i < active.size(); ++i) {
            for (std::size_t j = 0; j < active.size(); ++j) {
                reduced[i][j] = covarianceMatrix[active[i]][active[j]];
            }
        }

        const auto reducedResult =
            PortfolioOptimizer::minimumVariance(
                reduced, maxIterations, tolerance);

        result.weights.assign(n, 0.0);
        for (std::size_t i = 0; i < active.size(); ++i) {
            result.weights[active[i]] = reducedResult.weights[i];
        }
        result.expectedReturn =
            weightedReturn(result.weights, assetExpectedReturns);
        result.variance =
            PortfolioVolatility::variance(result.weights, covarianceMatrix);
        result.volatility = std::sqrt(std::max(0.0, result.variance));
        result.iterations = reducedResult.iterations;
        result.converged = reducedResult.converged;
        return result;
    }

    const double lipschitzBound = maxAbsoluteRowSum(covarianceMatrix);
    const double stepSize =
        lipschitzBound > 0.0 ? 1.0 / (2.0 * lipschitzBound) : 1.0;

    std::vector<double> weights = baseline.weights;
    result.converged = false;

    for (std::size_t iteration = 1; iteration <= maxIterations; ++iteration) {
        const std::vector<double> sigmaW =
            multiply(covarianceMatrix, weights);

        std::vector<double> candidate(n);
        for (std::size_t i = 0; i < n; ++i) {
            candidate[i] = weights[i] - 2.0 * stepSize * sigmaW[i];
        }

        candidate = projectToFeasible(
            candidate, assetExpectedReturns, targetReturn);

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

    result.weights = std::move(weights);
    result.expectedReturn =
        weightedReturn(result.weights, assetExpectedReturns);
    result.variance =
        PortfolioVolatility::variance(result.weights, covarianceMatrix);

    if (result.variance < 0.0 && result.variance > -1e-12) {
        result.variance = 0.0;
    }
    if (!std::isfinite(result.variance) || result.variance < 0.0) {
        throw std::runtime_error(
            "Optimized portfolio variance is invalid");
    }

    result.volatility = std::sqrt(result.variance);
    return result;
}

std::vector<EfficientFrontierPoint> EfficientFrontier::generate(
    const CovarianceMatrix::Matrix& covarianceMatrix,
    const std::vector<double>& assetExpectedReturns,
    std::size_t points,
    std::size_t maxIterations,
    double tolerance) {

    if (points == 0) {
        throw std::invalid_argument(
            "Frontier point count must be greater than zero");
    }

    const auto baseline =
        PortfolioOptimizer::minimumVariance(covarianceMatrix);
    validateReturns(assetExpectedReturns, covarianceMatrix.size());

    const double minimumFrontierReturn =
        weightedReturn(baseline.weights, assetExpectedReturns);
    const double maximumAssetReturn =
        *std::max_element(assetExpectedReturns.begin(),
                          assetExpectedReturns.end());

    if (maximumAssetReturn <= minimumFrontierReturn +
        1e-12 * std::max(1.0, std::abs(maximumAssetReturn))) {
        return {forTargetReturn(
            covarianceMatrix, assetExpectedReturns,
            minimumFrontierReturn, maxIterations, tolerance)};
    }

    if (points == 1) {
        return {forTargetReturn(
            covarianceMatrix, assetExpectedReturns,
            minimumFrontierReturn, maxIterations, tolerance)};
    }

    std::vector<EfficientFrontierPoint> frontier;
    frontier.reserve(points);

    for (std::size_t i = 0; i < points; ++i) {
        const double fraction =
            static_cast<double>(i) / static_cast<double>(points - 1);
        const double target =
            minimumFrontierReturn +
            fraction * (maximumAssetReturn - minimumFrontierReturn);

        frontier.push_back(forTargetReturn(
            covarianceMatrix, assetExpectedReturns, target,
            maxIterations, tolerance));
    }

    return frontier;
}

} // namespace ql
