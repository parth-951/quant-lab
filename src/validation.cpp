#include "validation.hpp"

#include <cmath>
#include <stdexcept>
#include <string>
namespace ql {

namespace {

void validateFinite(
    double value,
    const char* name
) {
    if (!std::isfinite(value)) {
        throw std::invalid_argument(
            std::string(name) + " must be finite"
        );
    }
}

} // anonymous namespace

void validateMarket(
    double spot,
    double rate,
    double volatility
) {
    // ------------------------------------------------------------
    // Spot
    // ------------------------------------------------------------

    validateFinite(spot, "Spot price");

    if (spot <= 0.0) {
        throw std::invalid_argument(
            "Spot price must be positive"
        );
    }

    // ------------------------------------------------------------
    // Risk-free rate
    //
    // Negative rates are allowed.
    // ------------------------------------------------------------

    validateFinite(rate, "Risk-free rate");

    // ------------------------------------------------------------
    // Volatility
    // ------------------------------------------------------------

    validateFinite(volatility, "Volatility");

    if (volatility <= 0.0) {
        throw std::invalid_argument(
            "Volatility must be positive"
        );
    }
}

void validateOption(
    double strike,
    double maturity
) {
    // ------------------------------------------------------------
    // Strike
    // ------------------------------------------------------------

    validateFinite(strike, "Strike price");

    if (strike <= 0.0) {
        throw std::invalid_argument(
            "Strike price must be positive"
        );
    }

    // ------------------------------------------------------------
    // Maturity
    // ------------------------------------------------------------

    validateFinite(maturity, "Maturity");

    if (maturity <= 0.0) {
        throw std::invalid_argument(
            "Maturity must be positive"
        );
    }
}

void validateSteps(int steps) {

    if (steps <= 0) {
        throw std::invalid_argument(
            "Number of steps must be positive"
        );
    }
}

void validateSimulations(int simulations) {
    if (simulations < 2) {
        throw std::invalid_argument(
            "Number of simulations must be at least 2"
        );
    }
}

} // namespace ql