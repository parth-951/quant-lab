#include "covariance_matrix.hpp"
#include "statistics.hpp"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

bool near(double actual, double expected, double tolerance = 1e-12) {
    return std::abs(actual - expected) <= tolerance;
}

template <typename Function>
void requireThrows(Function&& function, const std::string& message) {
    bool threw = false;
    try {
        function();
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    require(threw, message);
}

} // namespace

int main() {
    try {
        // 1. Known sample covariance matrix.
        const std::vector<std::vector<double>> returns{
            {1.0, 2.0, 3.0},
            {2.0, 4.0, 6.0}
        };
        const auto matrix = ql::CovarianceMatrix::calculate(returns);
        require(matrix.size() == 2 && matrix[0].size() == 2,
                "Matrix dimensions should be 2x2");
        require(near(matrix[0][0], 1.0), "Asset 0 variance should be 1");
        require(near(matrix[1][1], 4.0), "Asset 1 variance should be 4");
        require(near(matrix[0][1], 2.0), "Covariance should be 2");
        require(near(matrix[1][0], 2.0), "Covariance should be symmetric");

        // 2. Diagonal equals Statistics::sampleVariance for each asset.
        for (std::size_t i = 0; i < returns.size(); ++i) {
            require(near(matrix[i][i],
                         ql::Statistics::sampleVariance(returns[i])),
                    "Diagonal must match Statistics::sampleVariance");
        }

        // 3. Symmetry across a three-asset matrix.
        const std::vector<std::vector<double>> threeAssets{
            {0.01, -0.02, 0.03, 0.00},
            {0.02, -0.01, 0.01, 0.04},
            {-0.03, 0.02, 0.00, 0.01}
        };
        const auto symmetric = ql::CovarianceMatrix::calculate(threeAssets);
        for (std::size_t i = 0; i < symmetric.size(); ++i) {
            for (std::size_t j = 0; j < symmetric.size(); ++j) {
                require(near(symmetric[i][j], symmetric[j][i]),
                        "Covariance matrix must be symmetric");
            }
        }

        // 4. Constant series is valid and has zero variance/covariance.
        const auto constant = ql::CovarianceMatrix::calculate({
            {0.05, 0.05, 0.05},
            {0.01, 0.02, 0.03}
        });
        require(near(constant[0][0], 0.0),
                "Constant series variance should be zero");
        require(near(constant[0][1], 0.0),
                "Constant series covariance should be zero");

        // 5. Input validation.
        requireThrows([] {
            (void)ql::CovarianceMatrix::calculate({});
        }, "Empty asset list should throw");

        requireThrows([] {
            (void)ql::CovarianceMatrix::calculate({{0.01}});
        }, "Fewer than two observations should throw");

        requireThrows([] {
            (void)ql::CovarianceMatrix::calculate({
                {0.01, 0.02, 0.03},
                {0.04, 0.05}
            });
        }, "Mismatched observation counts should throw");

        requireThrows([] {
            (void)ql::CovarianceMatrix::calculate({
                {0.01, std::numeric_limits<double>::quiet_NaN()}
            });
        }, "NaN input should throw");

        requireThrows([] {
            (void)ql::CovarianceMatrix::calculate({
                {0.01, std::numeric_limits<double>::infinity()}
            });
        }, "Infinite input should throw");

        std::cout << "All covariance matrix tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Covariance matrix test failure: "
                  << error.what() << '\n';
        return 1;
    }
}
