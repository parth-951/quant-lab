#include "greeks.hpp"

#include <cmath>
#include <stdexcept>

namespace ql {

double Greeks::normalCDF(double x) const {
    return 0.5 * std::erfc(-x / std::sqrt(2.0));
}

double Greeks::normalPDF(double x) const {
    constexpr double PI = 3.14159265358979323846;

    return std::exp(-0.5 * x * x) /
           std::sqrt(2.0 * PI);
}

// ============================================================
// DELTA
// ============================================================

double Greeks::delta(
    const Option& option,
    const Market& market
) const {

    // --------------------------------------------------------
    // Validation
    // --------------------------------------------------------

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

    if (option.exercise != ExerciseStyle::European) {
        throw std::invalid_argument(
            "Black-Scholes Greeks support European options only"
        );
    }

    // --------------------------------------------------------
    // Parameters
    // --------------------------------------------------------

    const double S = market.spot;
    const double K = option.strike;
    const double r = market.rate;
    const double sigma = market.volatility;
    const double T = option.maturity;

    const double sqrtT = std::sqrt(T);

    const double d1 =
        (std::log(S / K) +
         (r + 0.5 * sigma * sigma) * T)
        / (sigma * sqrtT);

    // --------------------------------------------------------
    // Delta
    // --------------------------------------------------------

    if (option.type == OptionType::Call) {
        return normalCDF(d1);
    }

    return normalCDF(d1) - 1.0;
}

// ============================================================
// GAMMA
// ============================================================

double Greeks::gamma(
    const Option& option,
    const Market& market
) const {

    // --------------------------------------------------------
    // Validation
    // --------------------------------------------------------

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

    if (option.exercise != ExerciseStyle::European) {
        throw std::invalid_argument(
            "Black-Scholes Greeks support European options only"
        );
    }

    // --------------------------------------------------------
    // Parameters
    // --------------------------------------------------------

    const double S = market.spot;
    const double K = option.strike;
    const double r = market.rate;
    const double sigma = market.volatility;
    const double T = option.maturity;

    const double sqrtT = std::sqrt(T);

    const double d1 =
        (std::log(S / K) +
         (r + 0.5 * sigma * sigma) * T)
        / (sigma * sqrtT);

    // --------------------------------------------------------
    // Gamma
    // --------------------------------------------------------

    return normalPDF(d1) /
           (S * sigma * sqrtT);
}

// ============================================================
// VEGA
// ============================================================

double Greeks::vega(
    const Option& option,
    const Market& market
) const {

    // --------------------------------------------------------
    // Validation
    // --------------------------------------------------------

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

    if (option.exercise != ExerciseStyle::European) {
        throw std::invalid_argument(
            "Black-Scholes Greeks support European options only"
        );
    }

    // --------------------------------------------------------
    // Parameters
    // --------------------------------------------------------

    const double S = market.spot;
    const double K = option.strike;
    const double r = market.rate;
    const double sigma = market.volatility;
    const double T = option.maturity;

    const double sqrtT = std::sqrt(T);

    const double d1 =
        (std::log(S / K) +
         (r + 0.5 * sigma * sigma) * T)
        / (sigma * sqrtT);

    // --------------------------------------------------------
    // Vega
    // --------------------------------------------------------

    return S * normalPDF(d1) * sqrtT;
}

} // namespace ql