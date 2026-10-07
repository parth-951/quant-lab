
#pragma once

#include "market.hpp"
#include "option.hpp"

namespace ql {

class Greeks {
public:
    double delta(
        const Option& option,
        const Market& market
    ) const;

    double gamma(
        const Option& option,
        const Market& market
    ) const;

    double vega(
        const Option& option,
        const Market& market
    ) const;

private:
    double normalCDF(double x) const;
    double normalPDF(double x) const;
};

} // namespace ql