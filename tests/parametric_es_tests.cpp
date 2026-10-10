#include "parametric_es.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>

int main() {
    const ql::CovarianceMatrix::Matrix covariance{
        {0.0001, 0.0},
        {0.0, 0.0004}
    };

    const std::vector<double> weights{0.6, 0.4};

    const double es = ql::ParametricES::calculate(
        1'000'000.0, weights, covariance, 0.95);

    const double z = 1.6448536269514722;
    const double pi = std::acos(-1.0);
    const double expected =
        1'000'000.0 * 0.01
        * std::exp(-0.5 * z * z)
        / std::sqrt(2.0 * pi)
        / 0.05;

    if (std::abs(es - expected) > 1e-3) {
        std::cerr << "Parametric ES value mismatch\n";
        return 1;
    }

    const double fourDayES = ql::ParametricES::calculate(
        1'000'000.0, weights, covariance, 0.95, 4);

    if (std::abs(fourDayES - 2.0 * es) > 1e-3) {
        std::cerr << "Horizon scaling mismatch\n";
        return 1;
    }

    try {
        (void)ql::ParametricES::calculate(
            100.0, weights, covariance, 1.0);
        return 1;
    } catch (const std::invalid_argument&) {
        // Expected.
    }

    std::cout << "Parametric ES tests passed\n";
    return 0;
}