
#include "gbm.hpp"

#include <cmath>

namespace ql {

GBM::GBM(unsigned int seed)
    : rng_(seed) {}

std::vector<double> GBM::simulate(
    const Market& market,
    double maturity,
    int steps
) {
    std::vector<double> prices;
    prices.reserve(steps + 1);

    double dt = maturity / static_cast<double>(steps);
    double current = market.spot;

    prices.push_back(current);

    for (int i = 0; i < steps; ++i) {
        double z = rng_.normal();

        double drift =
            (market.rate - 0.5 * market.volatility * market.volatility) * dt;

        double diffusion =
            market.volatility * std::sqrt(dt) * z;

        current *= std::exp(drift + diffusion);

        prices.push_back(current);
    }

    return prices;
}

} // namespace ql