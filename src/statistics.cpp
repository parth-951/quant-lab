#include "statistics.hpp"

#include <cmath>
#include <stdexcept>
#include <string>

namespace ql {

void Statistics::validateNonEmptyFinite(
    const std::vector<double>& values
) {
    if (values.empty()) {
        throw std::invalid_argument(
            "Statistics input must contain at least one observation"
        );
    }

    for (std::size_t i = 0; i < values.size(); ++i) {
        if (!std::isfinite(values[i])) {
            throw std::invalid_argument(
                "Statistics input contains a non-finite value at index " +
                std::to_string(i)
            );
        }
    }
}

void Statistics::validatePairedFinite(
    const std::vector<double>& x,
    const std::vector<double>& y
) {
    if (x.size() != y.size()) {
        throw std::invalid_argument(
            "Paired statistical inputs must have equal lengths"
        );
    }

    if (x.size() < 2) {
        throw std::invalid_argument(
            "Paired statistical inputs must contain at least two observations"
        );
    }

    validateNonEmptyFinite(x);
    validateNonEmptyFinite(y);
}

double Statistics::mean(const std::vector<double>& values) {
    validateNonEmptyFinite(values);

    // Incremental mean reduces overflow risk compared with summing all values.
    double result = 0.0;
    std::size_t count = 0;

    for (const double value : values) {
        ++count;
        result += (value - result) / static_cast<double>(count);
    }

    if (!std::isfinite(result)) {
        throw std::overflow_error("Mean calculation produced a non-finite result");
    }

    return result;
}

double Statistics::sampleVariance(const std::vector<double>& values) {
    validateNonEmptyFinite(values);

    if (values.size() < 2) {
        throw std::invalid_argument(
            "Sample variance requires at least two observations"
        );
    }

    // Two-pass sample variance, using long double for intermediate sums.
    const double average = mean(values);
    long double squaredDeviationSum = 0.0L;

    for (const double value : values) {
        const long double deviation =
            static_cast<long double>(value) -
            static_cast<long double>(average);
        squaredDeviationSum += deviation * deviation;
    }

    const long double variance =
        squaredDeviationSum /
        static_cast<long double>(values.size() - 1);

    const double result = static_cast<double>(variance);

    if (!std::isfinite(result)) {
        throw std::overflow_error(
            "Sample variance calculation produced a non-finite result"
        );
    }

    return result;
}

double Statistics::standardDeviation(
    const std::vector<double>& values
) {
    return std::sqrt(sampleVariance(values));
}

double Statistics::covariance(
    const std::vector<double>& x,
    const std::vector<double>& y
) {
    validatePairedFinite(x, y);

    const double meanX = mean(x);
    const double meanY = mean(y);

    long double crossDeviationSum = 0.0L;

    for (std::size_t i = 0; i < x.size(); ++i) {
        const long double dx =
            static_cast<long double>(x[i]) -
            static_cast<long double>(meanX);
        const long double dy =
            static_cast<long double>(y[i]) -
            static_cast<long double>(meanY);

        crossDeviationSum += dx * dy;
    }

    const long double covarianceValue =
        crossDeviationSum /
        static_cast<long double>(x.size() - 1);

    const double result = static_cast<double>(covarianceValue);

    if (!std::isfinite(result)) {
        throw std::overflow_error(
            "Covariance calculation produced a non-finite result"
        );
    }

    return result;
}

double Statistics::correlation(
    const std::vector<double>& x,
    const std::vector<double>& y
) {
    validatePairedFinite(x, y);

    const double varianceX = sampleVariance(x);
    const double varianceY = sampleVariance(y);

    if (varianceX == 0.0 || varianceY == 0.0) {
        throw std::domain_error(
            "Correlation is undefined when either input has zero variance"
        );
    }

    const double covarianceXY = covariance(x, y);
    double result =
        covarianceXY / (std::sqrt(varianceX) * std::sqrt(varianceY));

    if (!std::isfinite(result)) {
        throw std::overflow_error(
            "Correlation calculation produced a non-finite result"
        );
    }

    // Protect against tiny floating-point excursions beyond [-1, 1].
    if (result > 1.0 && result < 1.0 + 1e-12) {
        result = 1.0;
    } else if (result < -1.0 && result > -1.0 - 1e-12) {
        result = -1.0;
    }

    if (result < -1.0 || result > 1.0) {
        throw std::runtime_error(
            "Correlation calculation fell outside [-1, 1]"
        );
    }

    return result;
}

} // namespace ql
