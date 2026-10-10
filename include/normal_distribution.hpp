
#pragma once

namespace ql {

class NormalDistribution {
public:
    // Inverse CDF of the standard normal distribution.
    // Requires 0 < probability < 1.
    [[nodiscard]] static double inverseCdf(double probability);

    // Probability density function of the standard normal distribution.
    [[nodiscard]] static double pdf(double x);
};

} // namespace ql
