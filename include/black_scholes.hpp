#pragma once

#include "market.hpp"
#include "option.hpp"

namespace ql {

class BlackScholes {
public:
    double price(
        const Option& option,
        const Market& market
    ) const;

private:
    double normalCDF(double x) const;
    double normalPDF(double x) const;
};

} // namespace ql