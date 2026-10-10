
#include "var_backtesting.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace ql {
namespace {

// Bernoulli log-likelihood, omitting zero-count terms.
double bernoulliLogLikelihood(
    std::size_t successes,
    std::size_t failures) {

    const std::size_t total = successes + failures;
    if (total == 0 || successes == 0 || failures == 0) {
        // If all observations are one state, the maximized
        // likelihood is 1 and its log-likelihood is zero.
        if (total == 0 || successes == 0 || failures == 0) {
            return 0.0;
        }
    }

    const double p =
        static_cast<double>(successes) / static_cast<double>(total);

    double result = 0.0;

    if (successes > 0) {
        result += static_cast<double>(successes) * std::log(p);
    }
    if (failures > 0) {
        result += static_cast<double>(failures) * std::log1p(-p);
    }

    return result;
}

double calculateKupiecLR(
    std::size_t observations,
    std::size_t breaches,
    double expectedBreachRate) {

    const double n = static_cast<double>(observations);
    const double x = static_cast<double>(breaches);
    const double observedRate = x / n;

    double difference = 0.0;

    if (breaches > 0) {
        difference += x * std::log(observedRate / expectedBreachRate);
    }

    const std::size_t nonBreaches = observations - breaches;

    if (nonBreaches > 0) {
        difference += static_cast<double>(nonBreaches)
            * std::log(
                (1.0 - observedRate) / (1.0 - expectedBreachRate));
    }

    return std::max(0.0, 2.0 * difference);
}

double calculateIndependenceLR(
    std::size_t n00,
    std::size_t n01,
    std::size_t n10,
    std::size_t n11) {

    const std::size_t transitions = n00 + n01 + n10 + n11;
    if (transitions == 0) {
        return 0.0;
    }

    // Null: breach probability is the same after either state.
    const std::size_t totalBreaches = n01 + n11;
    const std::size_t totalNonBreaches = n00 + n10;

    const double iidLogLikelihood =
        bernoulliLogLikelihood(totalBreaches, totalNonBreaches);

    // Alternative: transition probability depends on the previous state.
    const double markovLogLikelihood =
        bernoulliLogLikelihood(n01, n00)
        + bernoulliLogLikelihood(n11, n10);

    // Protect against tiny floating-point differences.
    return std::max(
        0.0, 2.0 * (markovLogLikelihood - iidLogLikelihood));
}

} // namespace

VaRBacktestResult VaRBacktesting::run(
    const std::vector<double>& predictedVaR,
    const std::vector<double>& realizedLosses,
    double confidenceLevel,
    double significanceLevel) {

    if (predictedVaR.empty() || realizedLosses.empty()) {
        throw std::invalid_argument(
            "Backtest series must not be empty.");
    }

    if (predictedVaR.size() != realizedLosses.size()) {
        throw std::invalid_argument(
            "Predicted VaR and realized losses must be time-aligned "
            "and equal in length.");
    }

    if (!std::isfinite(confidenceLevel)
        || confidenceLevel <= 0.5
        || confidenceLevel >= 1.0) {
        throw std::invalid_argument(
            "Confidence level must be greater than 0.5 and less than 1.0.");
    }

    if (!std::isfinite(significanceLevel)
        || significanceLevel <= 0.0
        || significanceLevel >= 1.0) {
        throw std::invalid_argument(
            "Significance level must be greater than 0 and less than 1.");
    }

    VaRBacktestResult result;
    result.observations = predictedVaR.size();
    result.expectedBreachRate = 1.0 - confidenceLevel;
    result.significanceLevel = significanceLevel;

    std::vector<bool> breaches;
    breaches.reserve(predictedVaR.size());

    for (std::size_t i = 0; i < predictedVaR.size(); ++i) {
        const double var = predictedVaR[i];
        const double loss = realizedLosses[i];

        if (!std::isfinite(var) || var < 0.0
            || !std::isfinite(loss) || loss < 0.0) {
            throw std::invalid_argument(
                "VaR thresholds and realized losses must be "
                "finite and nonnegative.");
        }

        const bool breached = loss > var;
        breaches.push_back(breached);

        if (breached) {
            ++result.breaches;
            result.breachIndices.push_back(i);
        }
    }

    result.observedBreachRate =
        static_cast<double>(result.breaches)
        / static_cast<double>(result.observations);

    // Kupiec unconditional coverage test: chi-square with 1 degree of freedom.
    result.kupiecLikelihoodRatio = calculateKupiecLR(
        result.observations,
        result.breaches,
        result.expectedBreachRate);

    result.kupiecPValue = std::erfc(
        std::sqrt(result.kupiecLikelihoodRatio / 2.0));

    result.rejectUnconditionalCoverage =
        result.kupiecPValue < significanceLevel;

    // Count first-order breach transitions.
    for (std::size_t i = 1; i < breaches.size(); ++i) {
        const bool previous = breaches[i - 1];
        const bool current = breaches[i];

        if (!previous && !current) {
            ++result.n00;
        } else if (!previous && current) {
            ++result.n01;
        } else if (previous && !current) {
            ++result.n10;
        } else {
            ++result.n11;
        }
    }

    // Christoffersen independence test: chi-square with 1 degree of freedom.
    result.independenceLikelihoodRatio = calculateIndependenceLR(
        result.n00, result.n01, result.n10, result.n11);

    result.independencePValue = std::erfc(
        std::sqrt(result.independenceLikelihoodRatio / 2.0));

    result.rejectIndependence =
        result.independencePValue < significanceLevel;

    // Conditional coverage combines unconditional coverage and independence.
    result.conditionalCoverageLikelihoodRatio = std::max(
        0.0,
        result.kupiecLikelihoodRatio
            + result.independenceLikelihoodRatio);

    // Chi-square with 2 degrees of freedom has survival function exp(-LR/2).
    result.conditionalCoveragePValue = std::exp(
        -result.conditionalCoverageLikelihoodRatio / 2.0);

    result.rejectConditionalCoverage =
        result.conditionalCoveragePValue < significanceLevel;

    return result;
}

} // namespace ql
