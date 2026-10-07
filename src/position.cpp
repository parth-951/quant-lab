#include "position.hpp"

#include <cmath>

namespace ql {

double Position::marketValue() const {
    return quantity * currentPrice;
}

double Position::pnl() const {
    return quantity * (currentPrice - entryPrice);
}

double Position::pnlPercent() const {
    const double initialValue = quantity * entryPrice;

    if (initialValue == 0.0) {
        return 0.0;
    }

    return pnl() / std::abs(initialValue);
}

}