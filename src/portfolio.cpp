#include "portfolio.hpp"

#include <cmath>
#include <stdexcept>

namespace ql {

void Portfolio::addPosition(const Position& position) {

    if (!std::isfinite(position.quantity)) {
        throw std::invalid_argument(
            "Position quantity must be finite"
        );
    }

    if (!std::isfinite(position.entryPrice) ||
        position.entryPrice <= 0.0) {
        throw std::invalid_argument(
            "Position entry price must be positive and finite"
        );
    }

    if (!std::isfinite(position.currentPrice) ||
        position.currentPrice <= 0.0) {
        throw std::invalid_argument(
            "Position current price must be positive and finite"
        );
    }

    if (position.instrumentId.empty()) {
        throw std::invalid_argument(
            "Position instrument ID must not be empty"
        );
    }

    

    positions_.push_back(position);
}

const std::vector<Position>& Portfolio::positions() const {
    return positions_;
}

double Portfolio::marketValue() const {
    double total = 0.0;

    for (const Position& position : positions_) {
        total += position.marketValue();
    }

    return total;
}

double Portfolio::pnl() const {
    double total = 0.0;

    for (const Position& position : positions_) {
        total += position.pnl();
    }

    return total;
}

} // namespace ql