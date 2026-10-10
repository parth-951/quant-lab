
#include "var_backtesting.hpp"

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

bool near(double actual, double expected, double tolerance = 1e-10) {
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
        // 1. Breach counting and Kupiec test.
        const auto result = ql::VaRBacktesting::run(
            {100.0, 100.0, 100.0, 100.0},
            {80.0, 101.0, 100.0, 140.0},
            0.95);

        require(result.observations == 4,
                "Incorrect observation count");

        require(result.breaches == 2,
                "Incorrect breach count");

        require(result.breachIndices == std::vector<std::size_t>{1, 3},
                "Incorrect breach indices");

        require(near(result.observedBreachRate, 0.5),
                "Incorrect observed breach rate");

        require(near(result.expectedBreachRate, 0.05),
                "Incorrect expected breach rate");

        require(near(result.significanceLevel, 0.05),
                "Default significance level should be 5%");

        require(result.kupiecLikelihoodRatio > 0.0,
                "Kupiec statistic should be positive for this sample");

        require(result.kupiecPValue >= 0.0
                    && result.kupiecPValue <= 1.0,
                "Kupiec p-value must be between zero and one");

        require(result.rejectUnconditionalCoverage,
                "Excessive breach frequency should reject coverage here");

        // 2. Loss equal to VaR is not a breach.
        const auto equalLoss = ql::VaRBacktesting::run(
            {100.0}, {100.0}, 0.95);

        require(equalLoss.breaches == 0,
                "Loss equal to VaR should not count as a breach");

        // 3. Zero breaches: statistic and p-value must remain valid.
        const auto zeroBreaches = ql::VaRBacktesting::run(
            std::vector<double>(20, 100.0),
            std::vector<double>(20, 50.0),
            0.95);

        require(zeroBreaches.breaches == 0,
                "Zero-breach sample should have zero breaches");

        require(zeroBreaches.kupiecLikelihoodRatio >= 0.0,
                "Zero-breach LR statistic should be nonnegative");

        require(zeroBreaches.kupiecPValue > 0.0
                    && zeroBreaches.kupiecPValue <= 1.0,
                "Zero-breach p-value should be valid");

        require(!zeroBreaches.rejectUnconditionalCoverage,
                "This zero-breach sample should not reject coverage");

        // 4. All breaches: calculation should remain finite and valid.
        const auto allBreaches = ql::VaRBacktesting::run(
            std::vector<double>(20, 100.0),
            std::vector<double>(20, 101.0),
            0.95);

        require(allBreaches.breaches == 20,
                "All-breach sample should count every observation");

        require(std::isfinite(allBreaches.kupiecLikelihoodRatio),
                "All-breach LR statistic should be finite");

        require(allBreaches.kupiecPValue >= 0.0
                    && allBreaches.kupiecPValue <= 1.0,
                "All-breach p-value should be valid");

        require(allBreaches.rejectUnconditionalCoverage,
                "All-breach sample should reject coverage");

        // 5. Custom significance level is recorded and applied.
        const auto customAlpha = ql::VaRBacktesting::run(
            {100.0, 100.0, 100.0, 100.0},
            {80.0, 101.0, 100.0, 140.0},
            0.95,
            0.01);

        require(near(customAlpha.significanceLevel, 0.01),
                "Custom significance level should be preserved");

        require(
            customAlpha.rejectUnconditionalCoverage
                == (customAlpha.kupiecPValue < 0.01),
            "Decision should use the specified significance level");

        // 5b. Christoffersen independence and conditional coverage.
        // Breaches occur in two consecutive clusters: 0,0,0,1,1,1.
        const auto clustered = ql::VaRBacktesting::run(
            std::vector<double>(6, 100.0),
            {50.0, 50.0, 50.0, 150.0, 150.0, 150.0},
            0.5 + 0.45);

        require(clustered.n00 == 2,
                "Incorrect zero-to-zero transition count");
        require(clustered.n01 == 1,
                "Incorrect zero-to-one transition count");
        require(clustered.n10 == 0,
                "Incorrect one-to-zero transition count");
        require(clustered.n11 == 2,
                "Incorrect one-to-one transition count");

        require(std::isfinite(clustered.independenceLikelihoodRatio)
                    && clustered.independenceLikelihoodRatio >= 0.0,
                "Independence LR must be finite and nonnegative");
        require(clustered.independencePValue >= 0.0
                    && clustered.independencePValue <= 1.0,
                "Independence p-value must be between zero and one");

        require(std::isfinite(clustered.conditionalCoverageLikelihoodRatio)
                    && clustered.conditionalCoverageLikelihoodRatio >= 0.0,
                "Conditional coverage LR must be finite and nonnegative");
        require(clustered.conditionalCoveragePValue >= 0.0
                    && clustered.conditionalCoveragePValue <= 1.0,
                "Conditional coverage p-value must be between zero and one");

        require(near(
                    clustered.conditionalCoverageLikelihoodRatio,
                    clustered.kupiecLikelihoodRatio
                        + clustered.independenceLikelihoodRatio),
                "Conditional coverage LR should combine both tests");

        require(
            clustered.rejectIndependence
                == (clustered.independencePValue
                    < clustered.significanceLevel),
            "Independence decision should use significance level");

        require(
            clustered.rejectConditionalCoverage
                == (clustered.conditionalCoveragePValue
                    < clustered.significanceLevel),
            "Conditional coverage decision should use significance level");

        // 5c. All observations have the same state: no transition instability.
        const auto noBreaches = ql::VaRBacktesting::run(
            std::vector<double>(10, 100.0),
            std::vector<double>(10, 50.0),
            0.95);


        require(noBreaches.independenceLikelihoodRatio >= 0.0
                    && std::isfinite(noBreaches.independenceLikelihoodRatio),
                "Independence LR should be finite and nonnegative");

        require(noBreaches.independencePValue >= 0.0
                    && noBreaches.independencePValue <= 1.0,
                "Independence p-value should be valid");

        require(noBreaches.conditionalCoveragePValue >= 0.0
                    && noBreaches.conditionalCoveragePValue <= 1.0,
                "Conditional coverage p-value should be valid");


        // 5d. One observation cannot form any transitions.
        const auto oneObservation = ql::VaRBacktesting::run(
            {100.0}, {50.0}, 0.95);

        require(oneObservation.n00 + oneObservation.n01
                    + oneObservation.n10 + oneObservation.n11 == 0,
                "One observation should have no transitions");
        require(near(oneObservation.independencePValue, 1.0),
                "One observation should have neutral independence p-value");

        // Reference case 1: clustered breaches 000111.
        // Expected transitions: n00=2, n01=1, n10=0, n11=2.
        const auto clusteredReference = ql::VaRBacktesting::run(
            std::vector<double>(6, 100.0),
            {50.0, 50.0, 50.0, 150.0, 150.0, 150.0},
            0.95);

        require(clusteredReference.n00 == 2
                    && clusteredReference.n01 == 1
                    && clusteredReference.n10 == 0
                    && clusteredReference.n11 == 2,
                "Clustered reference transition counts are incorrect");

        // Independently calculated from the Bernoulli log-likelihoods.
        require(near(
                    clusteredReference.independenceLikelihoodRatio,
                    2.911031660323688,
                    1e-8),
                "Clustered reference independence LR is incorrect");

        // Reference case 2: alternating breaches 010101.
        // Expected transitions: n00=0, n01=3, n10=2, n11=0.
        const auto alternatingReference = ql::VaRBacktesting::run(
            std::vector<double>(6, 100.0),
            {50.0, 150.0, 50.0, 150.0, 50.0, 150.0},
            0.95);

        require(alternatingReference.n00 == 0
                    && alternatingReference.n01 == 3
                    && alternatingReference.n10 == 2
                    && alternatingReference.n11 == 0,
                "Alternating reference transition counts are incorrect");

        require(near(
                    alternatingReference.independenceLikelihoodRatio,
                    6.730116670092564,
                    1e-8),
                "Alternating reference independence LR is incorrect");

        // Alternating outcomes also violate independence; the test detects
        // both positive clustering and negative serial dependence.
        require(
            alternatingReference.independenceLikelihoodRatio
                > clusteredReference.independenceLikelihoodRatio,
            "Alternating reference should have the larger independence LR");

        // Verify the conditional coverage statistic is the sum of both LRs.
        require(near(
                    clusteredReference.conditionalCoverageLikelihoodRatio,
                    clusteredReference.kupiecLikelihoodRatio
                        + clusteredReference.independenceLikelihoodRatio,
                    1e-8),
                "Clustered conditional coverage LR is incorrect");

        require(near(
                    alternatingReference.conditionalCoverageLikelihoodRatio,
                    alternatingReference.kupiecLikelihoodRatio
                        + alternatingReference.independenceLikelihoodRatio,
                    1e-8),
                "Alternating conditional coverage LR is incorrect");

        // 6. Invalid input checks.
        requireThrows([] {
            (void)ql::VaRBacktesting::run({}, {}, 0.95);
        }, "Empty series should be rejected");

        requireThrows([] {
            (void)ql::VaRBacktesting::run(
                {100.0}, {101.0, 102.0}, 0.95);
        }, "Unequal series lengths should be rejected");

        requireThrows([] {
            (void)ql::VaRBacktesting::run(
                {100.0}, {101.0}, 0.5);
        }, "Confidence level 0.5 should be rejected");

        requireThrows([] {
            (void)ql::VaRBacktesting::run(
                {100.0}, {101.0}, 1.0);
        }, "Confidence level 1.0 should be rejected");

        requireThrows([] {
            (void)ql::VaRBacktesting::run(
                {100.0}, {101.0},
                std::numeric_limits<double>::quiet_NaN());
        }, "NaN confidence level should be rejected");

        requireThrows([] {
            (void)ql::VaRBacktesting::run(
                {100.0}, {101.0}, 0.95, 0.0);
        }, "Zero significance level should be rejected");

        requireThrows([] {
            (void)ql::VaRBacktesting::run(
                {100.0}, {101.0}, 0.95, 1.0);
        }, "Significance level 1.0 should be rejected");

        requireThrows([] {
            (void)ql::VaRBacktesting::run(
                {-1.0}, {101.0}, 0.95);
        }, "Negative VaR should be rejected");

        requireThrows([] {
            (void)ql::VaRBacktesting::run(
                {100.0}, {-1.0}, 0.95);
        }, "Negative realized loss should be rejected");

        requireThrows([] {
            (void)ql::VaRBacktesting::run(
                {std::numeric_limits<double>::infinity()},
                {101.0}, 0.95);
        }, "Infinite VaR should be rejected");

        requireThrows([] {
            (void)ql::VaRBacktesting::run(
                {100.0},
                {std::numeric_limits<double>::quiet_NaN()},
                0.95);
        }, "NaN realized loss should be rejected");

        std::cout << "All VaR backtesting tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "VaR backtesting test failure: "
                  << error.what() << '\n';
        return 1;
    }
}
