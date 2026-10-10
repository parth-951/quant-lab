#include "binomial.hpp"
#include "black_scholes.hpp"
#include "gbm.hpp"
#include "greeks.hpp"
#include "implied_vol.hpp"
#include "monte_carlo.hpp"
#include "option.hpp"
#include "portfolio.hpp"
#include "portfolio_exposure.hpp"
#include "portfolio_historical_risk.hpp"
#include "risk_attribution.hpp"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>
int main() {
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
    // OPTIONS
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
        blackScholes.price(
            genericCall,
            market
        );
    const double blackScholesPut =
        blackScholes.price(
            genericPut,
            market
        );
    // ============================================================
    // BINOMIAL TREE
    // ============================================================
    const double europeanCall =
        binomial.price(
            genericCall,
            market,
            500
        );
    const double europeanPut =
        binomial.price(
            genericPut,
            market,
            500
        );
    const double americanCall =
        binomial.price(
            genericAmericanCall,
            market,
            500
        );
    const double americanPut =
        binomial.price(
            genericAmericanPut,
            market,
            500
        );
    // ============================================================
    // MONTE CARLO
    // ============================================================
    const MCResult monteCarloCall =
        monteCarlo.price(
            genericCall,
            market,
            100000
        );
    const MCResult monteCarloPut =
        monteCarlo.price(
            genericPut,
            market,
            100000
        );
    // ============================================================
    // GREEKS
    // ============================================================
    const double delta =
        greeks.delta(
            genericCall,
            market
        );
    const double gamma =
        greeks.gamma(
            genericCall,
            market
        );
    const double vega =
        greeks.vega(
            genericCall,
            market
        );
    // ============================================================
    // IMPLIED VOLATILITY
    // ============================================================
    const double impliedVol =
        impliedVolatility.solve(
            genericCall,
            market,
            blackScholesCall
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
    // PORTFOLIO
    // ============================================================
    Portfolio portfolio;
    portfolio.addPosition(
        Position{
            10.0,
            95.0,
            100.0,
            "A"
        }
    );
    portfolio.addPosition(
        Position{
            5.0,
            190.0,
            200.0,
            "B"
        }
    );
    portfolio.addPosition(
        Position{
            -3.0,
            110.0,
            105.0,
            "HEDGE"
        }
    );
    // ============================================================
    // PORTFOLIO EXPOSURE
    // ============================================================
    const PortfolioExposure exposureEngine;
    const PortfolioExposureResult exposure =
        exposureEngine.calculate(
            portfolio
        );
    // ============================================================
    // HISTORICAL PRICE DATA
    // ============================================================
    const std::vector<HistoricalPriceSeries> histories{
        {
            "A",
            {
                "2026-01-02",
                "2026-01-03",
                "2026-01-04",
                "2026-01-05"
            },
            {
                100.0,
                95.0,
                90.0,
                105.0
            }
        },
        {
            "B",
            {
                "2026-01-02",
                "2026-01-03",
                "2026-01-04",
                "2026-01-05"
            },
            {
                200.0,
                190.0,
                180.0,
                210.0
            }
        },
        {
            "HEDGE",
            {
                "2026-01-02",
                "2026-01-03",
                "2026-01-04",
                "2026-01-05"
            },
            {
                105.0,
                110.0,
                115.0,
                100.0
            }
        }
    };
    // ============================================================
    // HISTORICAL PORTFOLIO RISK
    // ============================================================
    const PortfolioHistoricalRisk historicalRiskEngine;
    const PortfolioHistoricalRiskResult historicalRisk =
        historicalRiskEngine.calculate(
            portfolio,
            histories,
            0.75
        );
    // ============================================================
    // RISK ATTRIBUTION
    // ============================================================
    const RiskAttribution attributionEngine;
    const RiskAttributionResult attribution =
        attributionEngine.calculate(
            historicalRisk,
            0.75
        );
    // ============================================================
    // OUTPUT
    // ============================================================
    std::cout
        << std::fixed
        << std::setprecision(6);
    std::cout
        << "========================================\n"
        << "              QUANT LAB\n"
        << "========================================\n\n";
    // ============================================================
    // BLACK-SCHOLES
    // ============================================================
    std::cout
        << "===== BLACK-SCHOLES =====\n";
    std::cout
        << "Call Price : "
        << blackScholesCall
        << '\n';
    std::cout
        << "Put Price  : "
        << blackScholesPut
        << "\n\n";
    // ============================================================
    // BINOMIAL TREE
    // ============================================================
    std::cout
        << "===== BINOMIAL TREE =====\n";
    std::cout
        << "European Call : "
        << europeanCall
        << '\n';
    std::cout
        << "European Put  : "
        << europeanPut
        << '\n';
    std::cout
        << "American Call : "
        << americanCall
        << '\n';
    std::cout
        << "American Put  : "
        << americanPut
        << '\n';

    std::cout
        << "Early Exercise Premium (Put) : "
        << americanPut - europeanPut
        << "\n\n";
    // ============================================================
    // MONTE CARLO
    // ============================================================
    std::cout
        << "===== MONTE CARLO =====\n";
    std::cout
        << "Call Price : "
        << monteCarloCall.price
        << '\n';
    std::cout
        << "Call 95% CI : ["
        << monteCarloCall.lower95
        << ", "
        << monteCarloCall.upper95
        << "]\n";
    std::cout
        << "Put Price : "
        << monteCarloPut.price
        << '\n';
    std::cout
        << "Put 95% CI : ["
        << monteCarloPut.lower95
        << ", "
        << monteCarloPut.upper95
        << "]\n\n";
    // ============================================================
    // GREEKS
    // ============================================================
    std::cout
        << "===== GREEKS =====\n";
    std::cout
        << "Delta : "
        << delta
        << '\n';
    std::cout
        << "Gamma : "
        << gamma
        << '\n';
    std::cout
        << "Vega  : "
        << vega
        << "\n\n";
    // ============================================================
    // IMPLIED VOLATILITY
    // ============================================================
    std::cout
        << "===== IMPLIED VOLATILITY =====\n";
    std::cout
        << "Implied Volatility : "
        << impliedVol * 100.0
        << "%\n\n";
    // ============================================================
    // PRICING COMPARISON
    // ============================================================
    std::cout
        << "===== PRICING COMPARISON =====\n";
    std::cout
        << "BS vs European Tree : "
        << std::abs(
            blackScholesCall -
            europeanCall
        )
        << '\n';
    std::cout
        << "BS vs Monte Carlo   : "
        << std::abs(
            blackScholesCall -
            monteCarloCall.price
        )
        << '\n';
    std::cout
        << "Put BS vs Tree      : "
        << std::abs(
            blackScholesPut -
            europeanPut
        )
        << '\n';
    std::cout
        << "Put BS vs MC        : "
        << std::abs(
            blackScholesPut -
            monteCarloPut.price
        )
        << "\n\n";
    // ============================================================
    // GBM
    // ============================================================
    std::cout
        << "===== GBM =====\n";
    std::cout
        << "First 10 simulated prices:\n";
    for (
        std::size_t i = 0;
        i < std::min<std::size_t>(10, path.size());
        ++i
    ) {
        std::cout
            << path[i]
            << '\n';
    }
    std::cout
        << "Terminal Price : "
        << path.back()
        << "\n\n";
    // ============================================================
    // PORTFOLIO EXPOSURE TABLE
    // ============================================================
    std::cout
        << "===== PORTFOLIO EXPOSURE =====\n";
    std::cout
        << std::left
        << std::setw(18)
        << "Instrument"
        << std::right
        << std::setw(18)
        << "Market Value"
        << std::setw(18)
        << "Gross Weight"
        << '\n';
    std::cout
        << std::string(54, '-')
        << '\n';
    for (const auto& instrument : exposure.instruments) {
        std::cout
            << std::left
            << std::setw(18)
            << instrument.instrumentId
            << std::right
            << std::setw(18)
            << instrument.marketValue
            << std::setw(17)
            << instrument.grossWeight * 100.0
            << "%\n";
    }
    std::cout
        << std::string(54, '-')
        << '\n';
    std::cout
        << std::left
        << std::setw(18)
        << "Net Exposure"
        << std::right
        << std::setw(18)
        << exposure.netExposure
        << '\n';
    std::cout
        << std::left
        << std::setw(18)
        << "Gross Exposure"
        << std::right
        << std::setw(18)
        << exposure.grossExposure
        << "\n\n";
    // ============================================================
    // HISTORICAL RISK
    // ============================================================
    std::cout
        << "===== HISTORICAL PORTFOLIO RISK =====\n";
    std::cout
        << "Current Market Value : "
        << historicalRisk.currentMarketValue
        << '\n';
    std::cout
        << "Current P&L          : "
        << historicalRisk.currentPnl
        << '\n';
    std::cout
        << "Historical VaR       : "
        << historicalRisk.var
        << '\n';
    std::cout
        << "Expected Shortfall   : "
        << historicalRisk.expectedShortfall
        << "\n\n";
    // ============================================================
    // COMPONENT ES TABLE
    // ============================================================
    std::cout
        << "===== COMPONENT ES =====\n";
    std::cout
        << std::left
        << std::setw(18)
        << "Instrument"
        << std::right
        << std::setw(18)
        << "Component ES"
        << std::setw(20)
        << "Contribution %"
        << '\n';
    std::cout
        << std::string(56, '-')
        << '\n';
    for (
        const auto& [instrumentId, componentES]
        : attribution.componentES
    ) {
        std::cout
            << std::left
            << std::setw(18)
            << instrumentId
            << std::right
            << std::setw(18)
            << componentES
            << std::setw(19)
            << attribution.contributionPercent.at(instrumentId)
            << "%\n";
    }
    std::cout
        << std::string(56, '-')
        << '\n';
    std::cout
        << std::left
        << std::setw(18)
        << "Total ES"
        << std::right
        << std::setw(18)
        << attribution.totalES
        << "\n\n";
    std::cout
        << "========================================\n";
    return 0;
}
