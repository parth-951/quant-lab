#pragma once

#include "position.hpp"

#include <vector>

namespace ql {

class Portfolio {
public:
    void addPosition(const Position& position);

    const std::vector<Position>& positions() const;

    double marketValue() const;
    double pnl() const;

private:
    std::vector<Position> positions_;
};

} // namespace ql