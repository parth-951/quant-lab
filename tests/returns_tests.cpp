#include "returns.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace {

void expectNear(
    std::string_view name,
    double actual,
    double expected,
    double tolerance
) {
    if (std::abs(actual - expected) > tolerance) {
        std::cerr
            << "FAILED: " << name
            << "\nExpected: " << expected
            << "\nActual: " << actual
            << "\nTolerance: " << tolerance
            << "\n";

        std::exit(EXIT_FAILURE);
    }
}

void expectTrue(
    std::string_view name,
    bool condition
) {
    if (!condition) {
        std::cerr
            << "FAILED: " << name
            << "\n";

        std::exit(EXIT_FAILURE);
    }
}

template <typename Function>
void expectThrows(
    std::string_view name,
    Function function
) {
    try {
        function();

        std::cerr
            << "FAILED: " << name
            << "\nExpected invalid_argument exception.\n";

        std::exit(EXIT_FAILURE);
    }
    catch (const std::invalid_argument&) {
        // Expected.
    }
}

} // namespace

int main() {

    // ============================================================
    // HISTORICAL RETURNS
    // ============================================================

    {
        const std::vector<double> prices{
            100.0,
            105.0,
            102.0,
            106.0
        };

        const auto returns = ql::historicalReturns(prices);

        expectTrue(
            "Historical returns size",
            returns.size() == 3
        );

        const double expected1 = 0.05;
        const double expected2 = (102.0 - 105.0) / 105.0;
        const double expected3 = (106.0 - 102.0) / 102.0;

        expectNear(
            "Historical return 1",
            returns[0],
            expected1,
            1e-12
        );

        expectNear(
            "Historical return 2",
            returns[1],
            expected2,
            1e-12
        );

        expectNear(
            "Historical return 3",
            returns[2],
            expected3,
            1e-12
        );
    }


    // ============================================================
    // VALIDATION TESTS
    // ============================================================

    expectThrows(
        "Historical returns reject insufficient prices",
        []() {
            ql::historicalReturns({100.0});
        }
    );

    expectThrows(
        "Historical returns reject zero price",
        []() {
            ql::historicalReturns({
                100.0,
                0.0
            });
        }
    );

    expectThrows(
        "Historical returns reject negative price",
        []() {
            ql::historicalReturns({
                100.0,
                -5.0
            });
        }
    );

    expectThrows(
        "Historical returns reject NaN price",
        []() {
            ql::historicalReturns({
                100.0,
                std::nan("")
            });
        }
    );


    // ============================================================
    // RESULTS
    // ============================================================

    std::cout << "All returns tests passed.\n";

    return 0;
}   