#pragma once

#include <algorithm>

namespace ql {

enum class OptionType {
    Call,
    Put
};

enum class ExerciseStyle {
    European,
    American
};

struct Option {
    double strike;
    double maturity;
    OptionType type;
    ExerciseStyle exercise;

    double payoff(double stockPrice) const {

        if (type == OptionType::Call) {
            return std::max(
                stockPrice - strike,
                0.0
            );
        }

        return std::max(
            strike - stockPrice,
            0.0
        );
    }
};

} // namespace ql