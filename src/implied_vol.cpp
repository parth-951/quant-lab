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
    // --------------------------------------------------------
    // Basic validation
    // --------------------------------------------------------

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

    if (!std::isfinite(marketPrice) || marketPrice <= 0.0) {
        throw std::invalid_argument(
            "Market option price must be greater than zero."
        );
    }

    if (!std::isfinite(initialGuess) || initialGuess <= 0.0) {
        throw std::invalid_argument(
            "Initial volatility guess must be greater than zero."
        );
    }

    // --------------------------------------------------------
    // No-arbitrage price bounds
    // --------------------------------------------------------

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
            std::max(S - discountedStrike, 0.0);

        upperBound = S;
    }
    else {
        // max(K*e^(-rT) - S, 0) <= P <= K*e^(-rT)
        lowerBound =
            std::max(discountedStrike - S, 0.0);

        upperBound = discountedStrike;
    }

    // Small tolerance for floating-point rounding.
    const double boundTolerance = 1e-12;

    if (marketPrice < lowerBound - boundTolerance ||
        marketPrice > upperBound + boundTolerance) {

        throw std::invalid_argument(
            "Market option price is outside "
            "the theoretical no-arbitrage bounds."
        );
    }

    // --------------------------------------------------------
    // Newton-Raphson setup
    // --------------------------------------------------------

    BlackScholes blackScholes;
    Greeks greeks;

    double sigma = initialGuess;

    constexpr int MAX_ITERATIONS = 100;
    constexpr double VEGA_EPSILON = 1e-12;
    constexpr double PRICE_TOLERANCE = 1e-8;

    // --------------------------------------------------------
    // Newton-Raphson
    // --------------------------------------------------------

    for (int iteration = 0;
         iteration < MAX_ITERATIONS;
         ++iteration) {

        // Make sure the current volatility is valid.
        if (!std::isfinite(sigma) || sigma <= 0.0) {
            throw std::runtime_error(
                "Implied volatility iteration produced "
                "an invalid volatility."
            );
        }

        market.volatility = sigma;

        const double modelPrice =
            blackScholes.price(option, market);

        const double priceError =
            modelPrice - marketPrice;

        // Converged.
        if (std::abs(priceError) < PRICE_TOLERANCE) {
            return sigma;
        }

        // ----------------------------------------------------
        // Vega check
        // ----------------------------------------------------

        const double vega =
            greeks.vega(option, market);

        if (!std::isfinite(vega) ||
            std::abs(vega) < VEGA_EPSILON) {

            throw std::runtime_error(
                "Implied volatility failed: "
                "Vega is too small for a stable Newton update."
            );
        }

        // ----------------------------------------------------
        // Newton update
        // ----------------------------------------------------

        const double nextSigma =
            sigma - priceError / vega;

        // Never allow Newton-Raphson to continue with
        // zero or negative volatility.
        if (!std::isfinite(nextSigma) ||
            nextSigma <= 0.0) {

            throw std::runtime_error(
                "Implied volatility Newton update "
                "produced a non-positive volatility."
            );
        }

        sigma = nextSigma;
    }

    // --------------------------------------------------------
    // Maximum iteration limit reached
    // --------------------------------------------------------

    throw std::runtime_error(
        "Implied volatility failed to converge "
        "within the maximum number of iterations."
    );
}

} // namespace ql