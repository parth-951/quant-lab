#include "black_scholes.hpp"

#include <cmath>
#include <stdexcept>

namespace ql {

double BlackScholes::normalCDF(double x) const {
    return 0.5 * std::erfc(-x / std::sqrt(2.0));
}

double BlackScholes::normalPDF(double x) const {
    constexpr double PI = 3.14159265358979323846;

    return std::exp(-0.5 * x * x) /
           std::sqrt(2.0 * PI);
}

double BlackScholes::price(
    const Option& option,
    const Market& market
) const {

    // ------------------------------------------------------------
    // Input validation
    // ------------------------------------------------------------

    if (market.spot <= 0.0) {
        throw std::invalid_argument(
            "Spot price must be positive"
        );
    }

    if (option.strike <= 0.0) {
        throw std::invalid_argument(
            "Strike price must be positive"
        );
    }

    if (option.maturity <= 0.0) {
        throw std::invalid_argument(
            "Maturity must be positive"
        );
    }

    if (market.volatility <= 0.0) {
        throw std::invalid_argument(
            "Volatility must be positive"
        );
    }

    // Black-Scholes only supports European options
    if (option.exercise != ExerciseStyle::European) {
        throw std::invalid_argument(
            "Black-Scholes supports European options only"
        );
    }

    // ------------------------------------------------------------
    // Market parameters
    // ------------------------------------------------------------

    const double S = market.spot;
    const double K = option.strike;
    const double r = market.rate;
    const double sigma = market.volatility;
    const double T = option.maturity;

    const double sqrtT = std::sqrt(T);

    // ------------------------------------------------------------
    // Black-Scholes parameters
    // ------------------------------------------------------------

    const double d1 =
        (std::log(S / K) +
         (r + 0.5 * sigma * sigma) * T)
        / (sigma * sqrtT);

    const double d2 =
        d1 - sigma * sqrtT;

    // ------------------------------------------------------------
    // Option pricing
    // ------------------------------------------------------------

    if (option.type == OptionType::Call) {

        return S * normalCDF(d1)
             - K * std::exp(-r * T) * normalCDF(d2);

    }

    // European Put
    return K * std::exp(-r * T) * normalCDF(-d2)
         - S * normalCDF(-d1);
}

} // namespace ql