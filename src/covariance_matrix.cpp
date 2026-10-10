#include "covariance_matrix.hpp"

#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>

namespace ql {

CovarianceMatrix::Matrix CovarianceMatrix::calculate(
    const std::vector<std::vector<double>>& assetReturns) {
    if (assetReturns.empty()) {
        throw std::invalid_argument(
            "Covariance matrix requires at least one asset");
    }

    const std::size_t observationCount = assetReturns.front().size();
    if (observationCount < 2) {
        throw std::invalid_argument(
            "Covariance matrix requires at least two observations per asset");
    }

    for (std::size_t asset = 0; asset < assetReturns.size(); ++asset) {
        if (assetReturns[asset].size() != observationCount) {
            throw std::invalid_argument(
                "All asset return series must have the same number of observations");
        }

        for (std::size_t observation = 0;
             observation < observationCount;
             ++observation) {
            if (!std::isfinite(assetReturns[asset][observation])) {
                throw std::invalid_argument(
                    "Asset return series contains a non-finite value");
            }
        }
    }

    const std::size_t assetCount = assetReturns.size();
    std::vector<long double> means(assetCount, 0.0L);

    for (std::size_t asset = 0; asset < assetCount; ++asset) {
        long double sum = 0.0L;
        for (double value : assetReturns[asset]) {
            sum += static_cast<long double>(value);
        }
        means[asset] = sum / static_cast<long double>(observationCount);
    }

    Matrix result(assetCount, std::vector<double>(assetCount, 0.0));

    for (std::size_t i = 0; i < assetCount; ++i) {
        for (std::size_t j = i; j < assetCount; ++j) {
            long double sumProducts = 0.0L;

            for (std::size_t k = 0; k < observationCount; ++k) {
                const long double centeredI =
                    static_cast<long double>(assetReturns[i][k]) - means[i];
                const long double centeredJ =
                    static_cast<long double>(assetReturns[j][k]) - means[j];
                sumProducts += centeredI * centeredJ;
            }

            const long double covariance =
                sumProducts / static_cast<long double>(observationCount - 1);
            const double value = static_cast<double>(covariance);

            if (!std::isfinite(value)) {
                throw std::overflow_error(
                    "Covariance calculation produced a non-finite result");
            }

            result[i][j] = value;
            result[j][i] = value;
        }
    }

    return result;
}

} // namespace ql
