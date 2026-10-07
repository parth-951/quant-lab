#include "returns.hpp"

#include <cmath>
#include <stdexcept>

namespace ql {

std::vector<double> historicalReturns(
    const std::vector<double>& prices
) {
    if (prices.size() < 2) {
        throw std::invalid_argument(
            "At least two prices are required "
            "to calculate historical returns."
        );
    }

    for (double price : prices) {

        if (!std::isfinite(price)) {
            throw std::invalid_argument(
                "Price series contains a non-finite value."
            );
        }

        if (price <= 0.0) {
            throw std::invalid_argument(
                "Historical prices must be positive."
            );
        }
    }

    std::vector<double> returns;

    returns.reserve(prices.size() - 1);

    for (std::size_t i = 1;
         i < prices.size();
         ++i) {

        const double previousPrice =
            prices[i - 1];

        const double currentPrice =
            prices[i];

        const double returnValue =
            (currentPrice - previousPrice)
            / previousPrice;

        returns.push_back(returnValue);
    }

    return returns;
}

} // namespace ql