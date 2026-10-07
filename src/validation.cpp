#include "validation.hpp"

#include <stdexcept>
#include <string>

namespace ql {

void validateMarket(double spot,
                   double rate,
                   double volatility) {

    if (spot <= 0.0) {
        throw std::invalid_argument(
            "Spot price must be greater than zero."
        );
    }

    if (volatility <= 0.0) {
        throw std::invalid_argument(
            "Volatility must be greater than zero."
        );
    }

    if (rate <= -1.0) {
        throw std::invalid_argument(
            "Interest rate must be greater than -100%."
        );
    }
}

void validateOption(double strike,
                    double maturity) {

    if (strike <= 0.0) {
        throw std::invalid_argument(
            "Strike price must be greater than zero."
        );
    }

    if (maturity <= 0.0) {
        throw std::invalid_argument(
            "Maturity must be greater than zero."
        );
    }
}

void validateSteps(int steps) {

    if (steps <= 0) {
        throw std::invalid_argument(
            "Number of binomial steps must be greater than zero."
        );
    }
}

void validateSimulations(int simulations) {

    if (simulations <2) {
        throw std::invalid_argument(
            "Number of simulations must be atleast two."
        );
    }
}

} // namespace ql