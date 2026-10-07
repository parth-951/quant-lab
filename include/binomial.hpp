#pragma once

#include "market.hpp"
#include "option.hpp"

namespace ql {

class BinomialTree {
public:

    double price(
        const Option& option,
        const Market& market,
        int steps
    ) const;
};

} // namespace ql