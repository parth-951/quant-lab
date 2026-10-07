#include "portfolio.hpp"

namespace ql {

void Portfolio::addPosition(const Position& position) {
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

} // namespace ql