#pragma once

#include <vector>

namespace ql {

class HistoricalVaR {
public:
    double calculate(
        const std::vector<double>& returns,
        double confidenceLevel
    ) const;
};

} // namespace ql