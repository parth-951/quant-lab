#pragma once

#include "market.hpp"
#include "option.hpp"

namespace ql {

class ImpliedVolatility {
public:

    double solve(
        const Option& option,
        Market market,
        double marketPrice,
        double initialGuess = 0.20
    ) const;
};

} // namespace ql