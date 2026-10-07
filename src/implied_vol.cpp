#include "implied_vol.hpp"
#include "black_scholes.hpp"
#include "greeks.hpp"
#include "validation.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace ql {

double ImpliedVolatility::solve(
    const Option& option,
    Market market,
    double marketPrice,
    double initialGuess
) const {

    // ============================================================
    // BASIC VALIDATION
    // ============================================================

    validateMarket(
        market.spot,
        market.rate,
        market.volatility
    );

    validateOption(
        option.strike,
        option.maturity
    );

    if (option.exercise != ExerciseStyle::European) {
        throw std::invalid_argument(
            "Implied volatility currently supports "
            "European options only."
        );
    }

    if (!std::isfinite(marketPrice) ||
        marketPrice <= 0.0) {

        throw std::invalid_argument(
            "Market option price must be greater than zero."
        );
    }

    if (!std::isfinite(initialGuess) ||
        initialGuess <= 0.0) {

        throw std::invalid_argument(
            "Initial volatility guess must be greater than zero."
        );
    }

    // ============================================================
    // NO-ARBITRAGE PRICE BOUNDS
    // ============================================================

    const double S = market.spot;
    const double K = option.strike;
    const double r = market.rate;
    const double T = option.maturity;

    const double discountedStrike =
        K * std::exp(-r * T);

    double lowerBound;
    double upperBound;

    if (option.type == OptionType::Call) {

        // max(S - K*e^(-rT), 0) <= C <= S

        lowerBound =
            std::max(
                S - discountedStrike,
                0.0
            );

        upperBound = S;

    } else {

        // max(K*e^(-rT) - S, 0) <= P <= K*e^(-rT)

        lowerBound =
            std::max(
                discountedStrike - S,
                0.0
            );

        upperBound = discountedStrike;
    }

    constexpr double BOUND_TOLERANCE = 1e-12;

    if (marketPrice <
            lowerBound - BOUND_TOLERANCE ||
        marketPrice >
            upperBound + BOUND_TOLERANCE) {

        throw std::invalid_argument(
            "Market option price is outside "
            "the theoretical no-arbitrage bounds."
        );
    }

    // ============================================================
    // OBJECTS
    // ============================================================

    BlackScholes blackScholes;
    Greeks greeks;

    // ============================================================
    // SOLVER PARAMETERS
    // ============================================================

    constexpr int NEWTON_MAX_ITERATIONS = 50;
    constexpr int BISECTION_MAX_ITERATIONS = 100;

    constexpr double PRICE_TOLERANCE = 1e-8;
    constexpr double VOLATILITY_TOLERANCE = 1e-10;

    constexpr double VEGA_EPSILON = 1e-12;

    constexpr double MIN_VOLATILITY = 1e-8;
    constexpr double INITIAL_UPPER_VOLATILITY = 1.0;
    constexpr double MAX_VOLATILITY = 10.0;

    // ============================================================
    // BUILD A VOLATILITY BRACKET
    // ============================================================

    double lowVolatility = MIN_VOLATILITY;
    double highVolatility = INITIAL_UPPER_VOLATILITY;

    market.volatility = lowVolatility;

    const double lowPrice =
        blackScholes.price(
            option,
            market
        );

    market.volatility = highVolatility;

    double highPrice =
        blackScholes.price(
            option,
            market
        );

    // Increase the upper volatility until the market
    // price is bracketed.

    while (highPrice < marketPrice &&
           highVolatility < MAX_VOLATILITY) {

        highVolatility *= 2.0;

        if (highVolatility > MAX_VOLATILITY) {
            highVolatility = MAX_VOLATILITY;
        }

        market.volatility = highVolatility;

        highPrice =
            blackScholes.price(
                option,
                market
            );
    }

    // ============================================================
    // CHECK WHETHER A FINITE IMPLIED VOLATILITY EXISTS
    // ============================================================

    if (marketPrice < lowPrice - PRICE_TOLERANCE ||
        marketPrice > highPrice + PRICE_TOLERANCE) {

        throw std::runtime_error(
            "Unable to bracket implied volatility."
        );
    }

    // ============================================================
    // NEWTON-RAPHSON
    // ============================================================

    double sigma =
        std::clamp(
            initialGuess,
            lowVolatility,
            highVolatility
        );

    for (int iteration = 0;
         iteration < NEWTON_MAX_ITERATIONS;
         ++iteration) {

        if (!std::isfinite(sigma) ||
            sigma <= 0.0) {

            break;
        }

        market.volatility = sigma;

        const double modelPrice =
            blackScholes.price(
                option,
                market
            );

        const double priceError =
            modelPrice - marketPrice;

        // --------------------------------------------------------
        // Converged
        // --------------------------------------------------------

        if (std::abs(priceError) <
            PRICE_TOLERANCE) {

            return sigma;
        }

        // --------------------------------------------------------
        // Maintain the volatility bracket
        // --------------------------------------------------------

        if (priceError > 0.0) {
            highVolatility = sigma;
        } else {
            lowVolatility = sigma;
        }

        // --------------------------------------------------------
        // Check bracket width
        // --------------------------------------------------------

        if ((highVolatility - lowVolatility) <
            VOLATILITY_TOLERANCE) {

            return 0.5 *
                   (lowVolatility + highVolatility);
        }

        // --------------------------------------------------------
        // Vega
        // --------------------------------------------------------

        const double vega =
            greeks.vega(
                option,
                market
            );

        if (!std::isfinite(vega) ||
            std::abs(vega) < VEGA_EPSILON) {

            break;
        }

        // --------------------------------------------------------
        // Newton update
        // --------------------------------------------------------

        const double nextSigma =
            sigma -
            priceError / vega;

        // If Newton wants to leave the valid bracket,
        // stop Newton and use bisection.

        if (!std::isfinite(nextSigma) ||
            nextSigma <= lowVolatility ||
            nextSigma >= highVolatility) {

            break;
        }

        sigma = nextSigma;
    }

    // ============================================================
    // BISECTION FALLBACK
    // ============================================================

    for (int iteration = 0;
         iteration < BISECTION_MAX_ITERATIONS;
         ++iteration) {

        const double midVolatility =
            0.5 *
            (lowVolatility + highVolatility);

        market.volatility = midVolatility;

        const double modelPrice =
            blackScholes.price(
                option,
                market
            );

        const double priceError =
            modelPrice - marketPrice;

        // --------------------------------------------------------
        // Converged by price
        // --------------------------------------------------------

        if (std::abs(priceError) <
            PRICE_TOLERANCE) {

            return midVolatility;
        }

        // --------------------------------------------------------
        // Converged by volatility interval
        // --------------------------------------------------------

        if ((highVolatility - lowVolatility) <
            VOLATILITY_TOLERANCE) {

            return midVolatility;
        }

        // --------------------------------------------------------
        // Update bracket
        // --------------------------------------------------------

        if (priceError > 0.0) {

            highVolatility =
                midVolatility;

        } else {

            lowVolatility =
                midVolatility;
        }
    }

    // ============================================================
    // FAILURE
    // ============================================================

    throw std::runtime_error(
        "Implied volatility failed to converge "
        "using Newton-Raphson and bisection."
    );
}

} // namespace ql