#include "portfolio_exposure.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string_view>

using namespace ql;

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

    // ========================================================
    // TEST 1: NET AND GROSS EXPOSURE
    // ========================================================

    {
        Portfolio portfolio;

        portfolio.addPosition({
            10.0,
            90.0,
            100.0,
            "AAPL"
        });

        portfolio.addPosition({
            -5.0,
            120.0,
            100.0,
            "TSLA"
        });

        PortfolioExposure exposure;

        const PortfolioExposureResult result =
            exposure.calculate(portfolio);

        // AAPL = +1000
        // TSLA = -500
        //
        // Net   = +500
        // Gross = 1500

        expectNear(
            "Net exposure",
            result.netExposure,
            500.0,
            1e-12
        );

        expectNear(
            "Gross exposure",
            result.grossExposure,
            1500.0,
            1e-12
        );

        expectTrue(
            "Should contain two instruments",
            result.instruments.size() == 2
        );
    }


    // ========================================================
    // TEST 2: PER-INSTRUMENT MARKET VALUE AND GROSS WEIGHT
    // ========================================================

    {
        Portfolio portfolio;

        portfolio.addPosition({
            10.0,
            90.0,
            100.0,
            "AAPL"
        });

        portfolio.addPosition({
            -5.0,
            120.0,
            100.0,
            "TSLA"
        });

        PortfolioExposure exposure;

        const PortfolioExposureResult result =
            exposure.calculate(portfolio);

        expectTrue(
            "First instrument should be AAPL",
            result.instruments[0].instrumentId == "AAPL"
        );

        expectNear(
            "AAPL market value",
            result.instruments[0].marketValue,
            1000.0,
            1e-12
        );

        expectNear(
            "AAPL gross weight",
            result.instruments[0].grossWeight,
            1000.0 / 1500.0,
            1e-12
        );

        expectTrue(
            "Second instrument should be TSLA",
            result.instruments[1].instrumentId == "TSLA"
        );

        expectNear(
            "TSLA market value",
            result.instruments[1].marketValue,
            -500.0,
            1e-12
        );

        expectNear(
            "TSLA gross weight",
            result.instruments[1].grossWeight,
            500.0 / 1500.0,
            1e-12
        );
    }


    // ========================================================
    // TEST 3: MULTIPLE LOTS OF SAME INSTRUMENT
    // ========================================================

    {
        Portfolio portfolio;

        portfolio.addPosition({
            10.0,
            90.0,
            100.0,
            "AAPL"
        });

        portfolio.addPosition({
            5.0,
            95.0,
            100.0,
            "AAPL"
        });

        portfolio.addPosition({
            -5.0,
            120.0,
            100.0,
            "TSLA"
        });

        PortfolioExposure exposure;

        const PortfolioExposureResult result =
            exposure.calculate(portfolio);

        // AAPL:
        // 10 * 100 = 1000
        //  5 * 100 =  500
        // Total AAPL = 1500
        //
        // TSLA = -500
        //
        // Net   = 1000
        // Gross = 2000

        expectNear(
            "Net exposure with multiple lots",
            result.netExposure,
            1000.0,
            1e-12
        );

        expectNear(
            "Gross exposure with multiple lots",
            result.grossExposure,
            2000.0,
            1e-12
        );

        expectTrue(
            "Multiple lots should aggregate into one instrument",
            result.instruments.size() == 2
        );

        expectNear(
            "Aggregated AAPL market value",
            result.instruments[0].marketValue,
            1500.0,
            1e-12
        );

        expectNear(
            "Aggregated AAPL gross weight",
            result.instruments[0].grossWeight,
            0.75,
            1e-12
        );
    }


    // ========================================================
    // TEST 4: GROSS WEIGHTS SUM TO ONE
    // ========================================================

    {
        Portfolio portfolio;

        portfolio.addPosition({
            10.0,
            100.0,
            100.0,
            "A"
        });

        portfolio.addPosition({
            -20.0,
            50.0,
            50.0,
            "B"
        });

        portfolio.addPosition({
            5.0,
            100.0,
            100.0,
            "C"
        });

        PortfolioExposure exposure;

        const PortfolioExposureResult result =
            exposure.calculate(portfolio);

        double weightSum = 0.0;

        for (const InstrumentExposure& instrument :
             result.instruments) {
            weightSum += instrument.grossWeight;
        }

        expectNear(
            "Gross weights should sum to one",
            weightSum,
            1.0,
            1e-12
        );
    }


    // ========================================================
    // TEST 5: EMPTY PORTFOLIO
    // ========================================================

    {
        Portfolio portfolio;

        PortfolioExposure exposure;

        expectThrows(
            "Empty portfolio should throw",
            [&]() {
                exposure.calculate(portfolio);
            }
        );
    }


    std::cout
        << "All Portfolio Exposure tests passed.\n";

    return 0;
}