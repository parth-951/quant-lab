#include "statistics.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void expectNear(
    const std::string& name,
    double actual,
    double expected,
    double tolerance = 1e-12
) {
    if (std::abs(actual - expected) > tolerance) {
        std::cerr << "FAILED: " << name
                  << " | expected " << expected
                  << " | got " << actual << '\n';
        std::exit(1);
    }
}

void expectTrue(const std::string& name, bool condition) {
    if (!condition) {
        std::cerr << "FAILED: " << name << '\n';
        std::exit(1);
    }
}

template <typename Exception, typename Function>
void expectThrows(const std::string& name, Function function) {
    try {
        function();
    } catch (const Exception&) {
        return;
    } catch (...) {
        std::cerr << "FAILED: " << name << " (wrong exception type)\n";
        std::exit(1);
    }

    std::cerr << "FAILED: " << name << " (no exception thrown)\n";
    std::exit(1);
}

} // namespace

int main() {
    // Mean
    expectNear(
        "Mean",
        ql::Statistics::mean({1.0, 2.0, 3.0, 4.0, 5.0}),
        3.0
    );

    // Sample variance and standard deviation:
    // [1, 2, 3] has mean 2, squared deviations sum 2,
    // so sample variance = 2 / (3 - 1) = 1.
    expectNear(
        "Sample variance",
        ql::Statistics::sampleVariance({1.0, 2.0, 3.0}),
        1.0
    );
    expectNear(
        "Sample standard deviation",
        ql::Statistics::standardDeviation({1.0, 2.0, 3.0}),
        1.0
    );

    // Covariance: y = 2x has sample covariance 2 * sample variance(x) = 2.
    expectNear(
        "Positive covariance",
        ql::Statistics::covariance(
            {1.0, 2.0, 3.0},
            {2.0, 4.0, 6.0}
        ),
        2.0
    );

    // Correlation
    expectNear(
        "Perfect positive correlation",
        ql::Statistics::correlation(
            {1.0, 2.0, 3.0},
            {2.0, 4.0, 6.0}
        ),
        1.0
    );
    expectNear(
        "Perfect negative correlation",
        ql::Statistics::correlation(
            {1.0, 2.0, 3.0},
            {6.0, 4.0, 2.0}
        ),
        -1.0
    );

    // Validation
    expectThrows<std::invalid_argument>(
        "Empty mean input rejected",
        [] { (void)ql::Statistics::mean({}); }
    );
    expectThrows<std::invalid_argument>(
        "Single-observation sample variance rejected",
        [] { (void)ql::Statistics::sampleVariance({1.0}); }
    );
    expectThrows<std::invalid_argument>(
        "Mismatched covariance lengths rejected",
        [] {
            (void)ql::Statistics::covariance(
                {1.0, 2.0},
                {1.0}
            );
        }
    );
    expectThrows<std::domain_error>(
        "Correlation with constant input rejected",
        [] {
            (void)ql::Statistics::correlation(
                {2.0, 2.0, 2.0},
                {1.0, 2.0, 3.0}
            );
        }
    );

    std::cout << "All Statistics tests passed.\n";
    return 0;
}
