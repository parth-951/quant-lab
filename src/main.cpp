#include "binomial.hpp"
#include "black_scholes.hpp"
#include "gbm.hpp"
#include "greeks.hpp"
#include "implied_vol.hpp"
#include "monte_carlo.hpp"
#include "option.hpp"

#include <iomanip>
#include <iostream>

int main() {

    using namespace ql;

    // ============================================================
    // MARKET
    // ============================================================

    const Market market{
        100.0,   // Spot
        0.05,    // Risk-free rate
        0.20     // Volatility
    };


    // ============================================================
    // OLD OPTION TYPES
    // Kept while we migrate the pricing engines
    // ============================================================


    // ============================================================
    // NEW GENERIC OPTIONS
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
    // ENGINES
    // ============================================================

    const BlackScholes blackScholes;
    const BinomialTree binomial;
    const Greeks greeks;
    const ImpliedVolatility impliedVolatility;
     MonteCarlo monteCarlo;

    GBM gbm(42);


    // ============================================================
    // BLACK-SCHOLES
    // ============================================================

    const double blackScholesCall =
        blackScholes.price(genericCall, market);

    const double blackScholesPut =
        blackScholes.price(genericPut, market);


// ============================================================
// BINOMIAL
// ============================================================

const double binomialEuropeanCall =
    binomial.price(
        genericCall,
        market,
        500
    );

const double binomialEuropeanPut =
    binomial.price(
        genericPut,
        market,
        500
    );

const double binomialAmericanCall =
    binomial.price(
        genericAmericanCall,
        market,
        500
    );

const double binomialAmericanPut =
    binomial.price(
        genericAmericanPut,
        market,
        500
    );


    // ============================================================
    // NEW GENERIC MONTE CARLO API
    // ============================================================

    const MCResult genericCallMC =
        monteCarlo.price(
            genericCall,
            market,
            100000
        );

    const MCResult genericPutMC =
        monteCarlo.price(
            genericPut,
            market,
            100000
        );


    // ============================================================
    // GBM
    // ============================================================

    const auto path =
        gbm.simulate(
            market,
            genericCall.maturity,
            252
        );


    // ============================================================
    // OUTPUT
    // ============================================================

    std::cout
        << std::fixed
        << std::setprecision(6);


    std::cout
        << "========================================\n"
        << "             QUANT LAB\n"
        << "========================================\n\n";


    // ------------------------------------------------------------
    // Black-Scholes
    // ------------------------------------------------------------

    std::cout
        << "===== BLACK-SCHOLES =====\n";

    std::cout
        << "European Call : "
        << blackScholesCall
        << '\n';

    std::cout
        << "European Put  : "
        << blackScholesPut
        << "\n\n";


    // ------------------------------------------------------------
    // Binomial
    // ------------------------------------------------------------

    std::cout
        << "===== BINOMIAL TREE =====\n";

    std::cout
        << "European Call : "
        << binomialEuropeanCall
        << '\n';

    std::cout
        << "European Put  : "
        << binomialEuropeanPut
        << '\n';

    std::cout
        << "American Call : "
        << binomialAmericanCall
        << '\n';

    std::cout
        << "American Put  : "
        << binomialAmericanPut
        << '\n';

    std::cout
        << "Early Exercise Premium : "
        << binomialAmericanPut - binomialEuropeanPut
        << "\n\n";


    // ------------------------------------------------------------
    // NEW GENERIC MONTE CARLO
    // ------------------------------------------------------------

    std::cout
        << "===== GENERIC MONTE CARLO =====\n";

    std::cout
        << "Generic Call : "
        << genericCallMC.price
        << '\n';

    std::cout
        << "Call 95% CI : ["
        << genericCallMC.lower95
        << ", "
        << genericCallMC.upper95
        << "]\n";

    std::cout
        << "Generic Put  : "
        << genericPutMC.price
        << '\n';

    std::cout
        << "Put 95% CI  : ["
        << genericPutMC.lower95
        << ", "
        << genericPutMC.upper95
        << "]\n\n";


    // ------------------------------------------------------------
// Greeks
// ------------------------------------------------------------

std::cout
    << "===== GREEKS =====\n";

std::cout
    << "Delta : "
    << greeks.delta(
        genericCall,
        market
    )
    << '\n';

std::cout
    << "Gamma : "
    << greeks.gamma(
        genericCall,
        market
    )
    << '\n';

std::cout
    << "Vega  : "
    << greeks.vega(
        genericCall,
        market
    )
    << "\n\n";
    // ------------------------------------------------------------
    // Implied Volatility
    // ------------------------------------------------------------

    std::cout
        << "===== IMPLIED VOLATILITY =====\n";

    std::cout
        << "Recovered Volatility : "
        << impliedVolatility.solve(
            genericCall,
            market,
            blackScholesCall
        ) * 100.0
        << "%\n\n";


    // ------------------------------------------------------------
    // Validation / comparison
    // ------------------------------------------------------------

    std::cout
        << "===== PRICING COMPARISON =====\n";

    std::cout
        << "BS Call vs Tree Error : "
        << std::abs(
            blackScholesCall -
            binomialEuropeanCall
        )
        << '\n';

    std::cout
        << "BS Call vs Generic MC Error : "
        << std::abs(
            blackScholesCall -
            genericCallMC.price
        )
        << '\n';

    std::cout
        << "BS Put vs Tree Error : "
        << std::abs(
            blackScholesPut -
            binomialEuropeanPut
        )
        << '\n';

    std::cout
        << "BS Put vs Generic MC Error : "
        << std::abs(
            blackScholesPut -
            genericPutMC.price
        )
        << "\n\n";


    // ------------------------------------------------------------
    // GBM
    // ------------------------------------------------------------

    std::cout
        << "===== GBM =====\n";

    std::cout
        << "Seeded GBM Terminal Price : "
        << path.back()
        << '\n';


    std::cout
        << "\n========================================\n";

    return 0;
}