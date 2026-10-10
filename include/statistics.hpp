#pragma once

#include <vector>

namespace ql {

class Statistics {
public:
    // Arithmetic mean. Requires at least one finite observation.
    static double mean(const std::vector<double>& values);

    // Unbiased sample variance (denominator n - 1). Requires n >= 2.
    static double sampleVariance(const std::vector<double>& values);

    // Sample standard deviation: sqrt(sampleVariance(values)).
    static double standardDeviation(const std::vector<double>& values);

    // Unbiased sample covariance (denominator n - 1).
    // Both series must have equal length and contain at least two values.
    static double covariance(
        const std::vector<double>& x,
        const std::vector<double>& y
    );

    // Pearson correlation coefficient. Both series must have equal length,
    // contain at least two values, and have non-zero sample variance.
    static double correlation(
        const std::vector<double>& x,
        const std::vector<double>& y
    );

private:
    static void validateNonEmptyFinite(const std::vector<double>& values);
    static void validatePairedFinite(
        const std::vector<double>& x,
        const std::vector<double>& y
    );
};

} // namespace ql
