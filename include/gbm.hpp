
#pragma once

#include <vector>
#include "market.hpp"
#include "random.hpp"

namespace ql {

class GBM {
public:
    explicit GBM(unsigned int seed = std::random_device{}());

    std::vector<double> simulate(
        const Market& market,
        double maturity,
        int steps
    );

private:
    Random rng_;
};

} // namespace ql