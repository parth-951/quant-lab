#include "monte_carlo.hpp"
#include "validation.hpp"

#include <cmath>
#include <stdexcept>

namespace ql {

MonteCarlo::MonteCarlo(unsigned int seed)
    : rng_(seed)
{
}

MCResult MonteCarlo::price(
    const Option& option,
    const Market& market,
    int simulations)
{
    validateMarket(
        market.spot,
        market.rate,
        market.volatility
    );

    validateOption(
        option.strike,
        option.maturity
    );

    validateSimulations(simulations);

    if (option.exercise != ExerciseStyle::European) {
        throw std::invalid_argument(
            "Current Monte Carlo engine only supports European options."
        );
    }

    const double S = market.spot;
    const double r = market.rate;
    const double sigma = market.volatility;
    const double T = option.maturity;

    const double discount = std::exp(-r * T);
    const double sqrtT = std::sqrt(T);

    double sum = 0.0;
    double sumSq = 0.0;

    for (int i = 0; i < simulations; ++i) {

        const double Z = rng_.normal();

        const double ST =
            S * std::exp(
                (r - 0.5 * sigma * sigma) * T
                + sigma * sqrtT * Z
            );

        const double payoff = option.payoff(ST);

        const double discountedPayoff =
            discount * payoff;

        sum += discountedPayoff;
        sumSq += discountedPayoff * discountedPayoff;
    }

    const double mean =
        sum / static_cast<double>(simulations);

    double variance =
        (sumSq / static_cast<double>(simulations))
        - mean * mean;

    if (variance < 0.0) {
        variance = 0.0;
    }

    const double stdError =
        std::sqrt(
            variance / static_cast<double>(simulations)
        );

    const double lower95 =
        mean - 1.96 * stdError;

    const double upper95 =
        mean + 1.96 * stdError;

    return {
        mean,
        stdError,
        lower95,
        upper95
    };
}


MCResult MonteCarlo::priceAntithetic(
    const Option& option,
    const Market& market,
    int simulations)
{
    validateMarket(
        market.spot,
        market.rate,
        market.volatility
    );

    validateOption(
        option.strike,
        option.maturity
    );

    validateSimulations(simulations);

    if (option.exercise != ExerciseStyle::European) {
        throw std::invalid_argument(
            "Current Monte Carlo engine only supports European options."
        );
    }

    const double S = market.spot;
    const double r = market.rate;
    const double sigma = market.volatility;
    const double T = option.maturity;

    const double discount = std::exp(-r * T);
    const double sqrtT = std::sqrt(T);

    double sum = 0.0;
    double sumSq = 0.0;

    for (int i = 0; i < simulations; ++i) {

        const double Z = rng_.normal();

        // First path: +Z
        const double ST1 =
            S * std::exp(
                (r - 0.5 * sigma * sigma) * T
                + sigma * sqrtT * Z
            );

        // Antithetic path: -Z
        const double ST2 =
            S * std::exp(
                (r - 0.5 * sigma * sigma) * T
                - sigma * sqrtT * Z
            );

        const double payoff1 =
            option.payoff(ST1);

        const double payoff2 =
            option.payoff(ST2);

        const double averagePayoff =
            0.5 * (payoff1 + payoff2);

        const double discountedPayoff =
            discount * averagePayoff;

        sum += discountedPayoff;
        sumSq += discountedPayoff * discountedPayoff;
    }

    const double mean =
        sum / static_cast<double>(simulations);

    double variance =
        (sumSq / static_cast<double>(simulations))
        - mean * mean;

    if (variance < 0.0) {
        variance = 0.0;
    }

    const double stdError =
        std::sqrt(
            variance / static_cast<double>(simulations)
        );

    const double lower95 =
        mean - 1.96 * stdError;

    const double upper95 =
        mean + 1.96 * stdError;

    return {
        mean,
        stdError,
        lower95,
        upper95
    };
}

} // namespace ql