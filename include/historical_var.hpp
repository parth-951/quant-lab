
#pragma once

#include <vector>

namespace ql {

// Historical quantile conventions.
//
// LinearInterpolation:
//   Uses h = (1 - confidenceLevel) * (N - 1)
//   and linearly interpolates between adjacent sorted returns.
//
// NearestRank:
//   Uses the empirical lower-tail rank
//   ceil((1 - confidenceLevel) * N) - 1,
//   bounded to the available observations.
enum class HistoricalQuantileConvention {
    LinearInterpolation,
    NearestRank
};

class HistoricalVaR {
public:
    // Defaults to LinearInterpolation to preserve existing results.
    double calculate(
        const std::vector<double>& returns,
        double confidenceLevel,
        HistoricalQuantileConvention convention =
            HistoricalQuantileConvention::LinearInterpolation
    ) const;
};

} // namespace ql
