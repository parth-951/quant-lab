#pragma once

namespace ql {

struct Position {
    double quantity;
    double price;

    double marketValue() const;
};

} // namespace ql