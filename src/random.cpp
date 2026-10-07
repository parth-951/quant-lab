#include "random.hpp"

namespace ql {

Random::Random(unsigned int seed)
    : rng_(seed),
      normalDist_(0.0, 1.0)
{
}

double Random::normal() {
    return normalDist_(rng_);
}

double Random::uniform(double a, double b) {
    std::uniform_real_distribution<double> dist(a, b);
    return dist(rng_);
}

} // namespace ql