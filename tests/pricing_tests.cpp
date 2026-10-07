#include "binomial.hpp"
#include "black_scholes.hpp"
#include "greeks.hpp"
#include "implied_vol.hpp"
#include "monte_carlo.hpp"
#include "option.hpp"
#include "validation.hpp"
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
    double tolerance)
{
    if (std::abs(actual - expected) > tolerance) {
        std::cerr
            << "FAILED: " << name
            << "\nExpected: " << expected
            << "\nActual:   " << actual
            << "\nTolerance: " << tolerance
            << "\n";

        std::exit(EXIT_FAILURE);
    }
}

void expectTrue(
    std::string_view name,
    bool condition)
{
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
    Function function)
{
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


int main()
{
    using namespace ql;

    // ============================================================
    // MARKET
    // ============================================================

    const Market market{
        100.0,
        0.05,
        0.20
    };


    // ============================================================
    // GENERIC OPTIONS
    // ============================================================

    const Option genericCall{
        100.0,
        1.0,
        OptionType::Call,
        ExerciseStyle::European
    };

    const Option genericPut{
        100.0,
        1.0,
        OptionType::Put,
        ExerciseStyle::European
    };

    const Option genericAmericanCall{
        100.0,
        1.0,
        OptionType::Call,
        ExerciseStyle::American
    };

    const Option genericAmericanPut{
        100.0,
        1.0,
        OptionType::Put,
        ExerciseStyle::American
    };
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

    const auto returns = historicalReturns(prices);

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
    // ENGINES
    // ============================================================

    const BlackScholes blackScholes;
    const BinomialTree binomial;
    const Greeks greeks;
    const ImpliedVolatility impliedVolatility;
    

    // Fixed seed makes Monte Carlo tests deterministic.
    MonteCarlo monteCarlo(12345);

    const double blackScholesCall =
        blackScholes.price(genericCall, market);
    // ============================================================
    // BLACK-SCHOLES
    // ============================================================

    const double bsCall =
        blackScholes.price(
            genericCall,
            market
        );

    const double bsPut =
        blackScholes.price(
            genericPut,
            market
        );

    expectNear(
        "Black-Scholes call",
        bsCall,
        10.4505835722,
        1e-10
    );

    expectNear(
        "Black-Scholes put",
        bsPut,
        5.5735260223,
        1e-10
    );


    // ============================================================
    // OPTION PAYOFF
    // ============================================================

    expectNear(
        "Call payoff ITM",
        genericCall.payoff(120.0),
        20.0,
        1e-12
    );

    expectNear(
        "Call payoff OTM",
        genericCall.payoff(80.0),
        0.0,
        1e-12
    );

    expectNear(
        "Put payoff ITM",
        genericPut.payoff(80.0),
        20.0,
        1e-12
    );

    expectNear(
        "Put payoff OTM",
        genericPut.payoff(120.0),
        0.0,
        1e-12
    );


    // ============================================================
    // BINOMIAL TREE
    // ============================================================

    const double binomialEuropeanCall =
        binomial.price(
            genericCall,
            market,
            1000
        );

    const double binomialEuropeanPut =
        binomial.price(
            genericPut,
            market,
            1000
        );

    const double binomialAmericanCall =
        binomial.price(
            genericAmericanCall,
            market,
            1000
        );

    const double binomialAmericanPut =
        binomial.price(
            genericAmericanPut,
            market,
            1000
        );

    expectNear(
        "Binomial European call",
        binomialEuropeanCall,
        bsCall,
        0.02
    );

    expectNear(
        "Binomial European put",
        binomialEuropeanPut,
        bsPut,
        0.02
    );

    expectNear(
        "American call without dividends",
        binomialAmericanCall,
        bsCall,
        0.02
    );

    expectTrue(
        "American put has early exercise value",
        binomialAmericanPut >= binomialEuropeanPut
    );


    // ============================================================
    // GREEKS
    // ============================================================

    const double callDelta =
        greeks.delta(
            genericCall,
            market
        );

    const double callGamma =
        greeks.gamma(
            genericCall,
            market
        );

    const double callVega =
        greeks.vega(
            genericCall,
            market
        );

    expectNear(
        "Call delta",
        callDelta,
        0.6368306512,
        1e-10
    );

    expectNear(
        "Gamma",
        callGamma,
        0.0187620173,
        1e-10
    );

    expectNear(
        "Vega",
        callVega,
        37.52403469,
        1e-8
    );

    const double putDelta =
        greeks.delta(
            genericPut,
            market
        );

    expectNear(
        "Put delta",
        putDelta,
        -0.3631693489,
        1e-10
    );


    // ============================================================
    // IMPLIED VOLATILITY
    // ============================================================

    const double recoveredCallVol =
        impliedVolatility.solve(
            genericCall,
            market,
            bsCall
        );

    const double recoveredPutVol =
        impliedVolatility.solve(
            genericPut,
            market,
            bsPut
        );

    expectNear(
        "Call implied volatility",
        recoveredCallVol,
        0.20,
        1e-8
    );

    expectNear(
        "Put implied volatility",
        recoveredPutVol,
        0.20,
        1e-8
    );


    // ============================================================
    // MONTE CARLO
    // ============================================================

    const MCResult callMC =
    monteCarlo.price(
        genericCall,
        market,
        100000
    );

const MCResult putMC =
    monteCarlo.price(
        genericPut,
        market,
        100000
    );

expectTrue(
    "MC call confidence interval contains BS price",
    callMC.lower95 <= bsCall &&
    bsCall <= callMC.upper95
);

expectTrue(
    "MC put confidence interval contains BS price",
    putMC.lower95 <= bsPut &&
    bsPut <= callMC.upper95
);


// ============================================================
// MONTE CARLO — SIMULATION VALIDATION
// ============================================================

expectThrows(
    "Monte Carlo rejects one simulation",
    [&]() {
        monteCarlo.price(
            genericCall,
            market,
            1
        );
    }
);

expectThrows(
    "Monte Carlo rejects zero simulations",
    [&]() {
        monteCarlo.price(
            genericCall,
            market,
            0
        );
    }
);

expectThrows(
    "Monte Carlo rejects negative simulations",
    [&]() {
        monteCarlo.price(
            genericCall,
            market,
            -1
        );
    }
);

    // ============================================================
    // ANTITHETIC MONTE CARLO
    // ============================================================

    const MCResult antitheticCallMC =
        monteCarlo.priceAntithetic(
            genericCall,
            market,
            100000
        );

    expectTrue(
        "Antithetic MC confidence interval contains BS price",
        antitheticCallMC.lower95 <= bsCall &&
        bsCall <= antitheticCallMC.upper95
    );


    // ============================================================
    // MONTE CARLO — AMERICAN OPTION REJECTION
    // ============================================================

    expectThrows(
        "Monte Carlo rejects American call",
        [&]() {
            monteCarlo.price(
                genericAmericanCall,
                market,
                10000
            );
        }
    );

    expectThrows(
        "Monte Carlo rejects American put",
        [&]() {
            monteCarlo.price(
                genericAmericanPut,
                market,
                10000
            );
        }
    );


// ============================================================
// VALIDATION TESTS
// ============================================================

// Black-Scholes rejects zero spot
expectThrows(
    "Black-Scholes rejects zero spot",
    [&]() {
        Market invalidMarket{0.0, 0.05, 0.20};
        blackScholes.price(genericCall, invalidMarket);
    }
);

// ... your other Black-Scholes tests ...

// Greeks rejects zero spot
expectThrows(
    "Greeks reject zero spot",
    [&]() {
        Market invalidMarket{0.0, 0.05, 0.20};
        greeks.delta(genericCall, invalidMarket);
    }
);

// ... your other Greeks tests ...

// IV rejects zero market price
expectThrows(
    "IV rejects zero market price",
    [&]() {
        impliedVolatility.solve(
            genericCall,
            market,
            0.0
        );
    }
);

// IV rejects negative initial guess
expectThrows(
    "IV rejects negative initial guess",
    [&]() {
        impliedVolatility.solve(
            genericCall,
            market,
            blackScholesCall,
            -0.20
        );
    }
);

// IV rejects American options
expectThrows(
    "IV rejects American options",
    [&]() {
        impliedVolatility.solve(
            genericAmericanCall,
            market,
            blackScholesCall
        );
    }
);


// ============================================================
// ADD THE NEW IV BOUND TESTS HERE
// ============================================================

// IV rejects a call price above the spot price
expectThrows(
    "IV rejects call price above upper bound",
    [&]() {
        impliedVolatility.solve(
            genericCall,
            market,
            101.0
        );
    }
);

// IV rejects a call price below its lower bound
expectThrows(
    "IV rejects call price below lower bound",
    [&]() {
        Market highRateMarket{100.0, 0.50, 0.20};

        impliedVolatility.solve(
            genericCall,
            highRateMarket,
            1.0
        );
    }
);

// IV rejects a put price above discounted strike
expectThrows(
    "IV rejects put price above upper bound",
    [&]() {
        impliedVolatility.solve(
            genericPut,
            market,
            100.0
        );
    }
);

// IV rejects a put price below its lower bound
    expectThrows(
        "IV rejects put price below lower bound",
        [&]() {
            Option expensivePut{
                200.0,
                1.0,
                OptionType::Put,
                ExerciseStyle::European
            };

            impliedVolatility.solve(
                expensivePut,
                market,
                80.0
            );
        }
    );
    // Less than two prices
try {

    historicalReturns({100.0});

    std::cerr
        << "FAILED: Historical returns reject "
           "insufficient prices\n";

    return 1;

} catch (const std::invalid_argument&) {
}

// Zero price
try {

    historicalReturns({
        100.0,
        0.0
    });

    std::cerr
        << "FAILED: Historical returns reject "
           "zero price\n";

    return 1;

} catch (const std::invalid_argument&) {
}

// Negative price
try {

    historicalReturns({
        100.0,
        -5.0
    });

    std::cerr
        << "FAILED: Historical returns reject "
           "negative price\n";

    return 1;

} catch (const std::invalid_argument&) {
}

// NaN price
try {

    historicalReturns({
        100.0,
        std::nan("")
    });

    std::cerr
        << "FAILED: Historical returns reject "
           "NaN price\n";

    return 1;

} catch (const std::invalid_argument&) {
}
    // ============================================================
    // RESULTS
    // ============================================================

    std::cout << "All pricing tests passed.\n";

    return 0;
}