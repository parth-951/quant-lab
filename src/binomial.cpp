#include "binomial.hpp"
#include "validation.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>

namespace ql {

double BinomialTree::price(
    const Option& option,
    const Market& market,
    int steps) const {

    // ------------------------------------------------------------
    // Validation
    // ------------------------------------------------------------

    validateMarket(
        market.spot,
        market.rate,
        market.volatility
    );

    validateOption(
        option.strike,
        option.maturity
    );

    validateSteps(steps);


    // ------------------------------------------------------------
    // Monte Carlo-style restriction does NOT apply here.
    //
    // Binomial supports:
    // European
    // American
    // ------------------------------------------------------------


    const double S = market.spot;
    const double r = market.rate;
    const double sigma = market.volatility;
    const double T = option.maturity;


    // ------------------------------------------------------------
    // CRR parameters
    // ------------------------------------------------------------

    const double dt =
        T / static_cast<double>(steps);

    const double sqrtDt =
        std::sqrt(dt);

    const double u =
        std::exp(sigma * sqrtDt);

    const double d =
        1.0 / u;

    const double discount =
        std::exp(-r * dt);

    const double p =
        (std::exp(r * dt) - d)
        / (u - d);


    // ------------------------------------------------------------
    // Risk-neutral probability validation
    // ------------------------------------------------------------

    if (p < 0.0 || p > 1.0) {

        throw std::invalid_argument(
            "Invalid binomial parameters: "
            "risk-neutral probability is outside [0, 1]."
        );
    }


    // ------------------------------------------------------------
    // Terminal stock prices and payoffs
    // ------------------------------------------------------------

    std::vector<double> values(
        steps + 1
    );

    for (int j = 0; j <= steps; ++j) {

        const double stockPrice =
            S *
            std::pow(u, j) *
            std::pow(d, steps - j);

        values[j] =
            option.payoff(stockPrice);
    }


    // ------------------------------------------------------------
    // Backward induction
    // ------------------------------------------------------------

    for (int i = steps - 1; i >= 0; --i) {

        for (int j = 0; j <= i; ++j) {

            const double continuationValue =
                discount *
                (
                    p * values[j + 1]
                    +
                    (1.0 - p) * values[j]
                );


            // European option:
            // value = continuation value
            //
            // American option:
            // value = max(continuation, immediate exercise)

            if (option.exercise == ExerciseStyle::American) {

                const double stockPrice =
                    S *
                    std::pow(u, j) *
                    std::pow(d, i - j);

                const double exerciseValue =
                    option.payoff(stockPrice);

                values[j] =
                    std::max(
                        continuationValue,
                        exerciseValue
                    );

            } else {

                values[j] =
                    continuationValue;
            }
        }
    }


    return values[0];
}

} // namespace ql