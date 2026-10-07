#pragma once

#include <random>

namespace ql {

class Random {
public:
    explicit Random(
        unsigned int seed = std::random_device{}()
    );

    double normal();

    double uniform(double a, double b);

private:
    std::mt19937 rng_;
    std::normal_distribution<double> normalDist_;
};

} // namespace ql