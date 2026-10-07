#pragma once

#include <string>

namespace ql {

struct Position {
    double quantity;
    double entryPrice;
    double currentPrice;
    std::string instrumentId{};

    double marketValue() const;
    double pnl() const;
    double pnlPercent() const;
};

} // namespace ql