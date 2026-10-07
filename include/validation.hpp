#pragma once

namespace ql {

void validateMarket(double spot,
                   double rate,
                   double volatility);

void validateOption(double strike,
                    double maturity);

void validateSteps(int steps);

void validateSimulations(int simulations);

} // namespace ql