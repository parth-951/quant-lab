
#pragma once

#include <cstddef>
#include <vector>

namespace ql {

struct VaRBacktestResult {
    std::size_t observations = 0;
    std::size_t breaches = 0;
    double observedBreachRate = 0.0;
    double expectedBreachRate = 0.0;
    std::vector<std::size_t> breachIndices;

    // Kupiec unconditional coverage test.
    double kupiecLikelihoodRatio = 0.0;
    double kupiecPValue = 1.0;
    double significanceLevel = 0.05;
    bool rejectUnconditionalCoverage = false;

    // Christoffersen independence test.
    double independenceLikelihoodRatio = 0.0;
    double independencePValue = 1.0;
    bool rejectIndependence = false;

    // Christoffersen conditional coverage test.
    double conditionalCoverageLikelihoodRatio = 0.0;
    double conditionalCoveragePValue = 1.0;
    bool rejectConditionalCoverage = false;

    // Breach transition counts:
    // n00: no breach followed by no breach
    // n01: no breach followed by breach
    // n10: breach followed by no breach
    // n11: breach followed by breach
    std::size_t n00 = 0;
    std::size_t n01 = 0;
    std::size_t n10 = 0;
    std::size_t n11 = 0;
};

class VaRBacktesting {
public:
    // All values are nonnegative monetary losses.
    // A breach occurs when realizedLoss > predictedVaR.
    // Both input vectors must be time-aligned and equal in length.
    //
    // Performs:
    // 1. Kupiec unconditional coverage test (chi-square, 1 df).
    // 2. Christoffersen independence test (chi-square, 1 df).
    // 3. Conditional coverage test (chi-square, 2 df).
    //
    // Independence and conditional coverage need at least two
    // observations to form a transition. With fewer observations,
    // their statistics remain zero and p-values remain one.
    [[nodiscard]] static VaRBacktestResult run(
        const std::vector<double>& predictedVaR,
        const std::vector<double>& realizedLosses,
        double confidenceLevel,
        double significanceLevel = 0.05);
};

} // namespace ql
