#pragma once

#include <vector>

namespace ql {

// Computes the sample covariance matrix from historical return series.
// Each inner vector represents one asset; observations must be aligned by index.
// The returned matrix is square, symmetric, and uses denominator (n - 1).
class CovarianceMatrix {
public:
    using Matrix = std::vector<std::vector<double>>;

    [[nodiscard]] static Matrix calculate(
        const std::vector<std::vector<double>>& assetReturns);
};

} // namespace ql
