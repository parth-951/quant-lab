#pragma once

#include "market.hpp"
#include "option.hpp"
#include "random.hpp"
#include <random>
namespace ql {

struct MCResult {
    double price;
    double stdError;
    double lower95;
    double upper95;
};

class MonteCarlo {
public:
    explicit MonteCarlo(
        unsigned int seed = std::random_device{}()
    );

    MCResult price(
        const Option& option,
        const Market& market,
        int simulations
    );

    MCResult priceAntithetic(
        const Option& option,
        const Market& market,
        int simulations
    );

private:
    Random rng_;
};

} // namespace ql