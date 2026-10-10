#include "risk_attribution.hpp"
#include <cmath>
#include <iostream>
#include <string>
#include <vector>
#include <exception>
#include <stdexcept>
#include <cstdlib>
namespace {
void expectNear(
    const std::string& name,
    double actual,
    double expected,
    double tolerance
) {
    if (std::abs(actual - expected) > tolerance) {
        std::cerr << "FAILED: " << name
                  << " | expected " << expected
                  << " | got " << actual
                  << '\n';
        std::exit(1);
    }
}
void expectTrue(
    const std::string& name,
    bool condition
) {
    if (!condition) {
        std::cerr << "FAILED: " << name << '\n';
        std::exit(1);
    }
}
template <typename Function>
void expectInvalidArgument(
    const std::string& name,
    Function function,
    const std::string& expectedMessage = {}
) {
    try {
        function();
    } catch (const std::invalid_argument& e) {
        if (!expectedMessage.empty() &&
            std::string(e.what()).find(expectedMessage) == std::string::npos) {
            std::cerr << "FAILED: " << name
                      << " | exception message did not contain: "
                      << expectedMessage << '\n';
            std::exit(1);
        }
        return;
    } catch (...) {
        std::cerr << "FAILED: " << name
                  << " (wrong exception type)\n";
        std::exit(1);
    }

    std::cerr << "FAILED: " << name
              << " (no std::invalid_argument thrown)\n";
    std::exit(1);
}
} // namespace
int main() {
    try{
    // ============================================================
    // Test 1: Basic risk attribution
    //
    // Three scenarios, 80% confidence -> worst 1 scenario.
    //
    // Total worst scenario:
    //     -100
    //
    // Instrument A:
    //     -70
    //
    // Instrument B:
    //     -30
    //
    // Therefore:
    //     A ES = 70
    //     B ES = 30
    //     Total = 100
    // ============================================================
    {
        ql::PortfolioHistoricalRiskResult riskResult{
            1000.0,
            0.0,
            {
                "2026-01-02",
                "2026-01-03",
                "2026-01-04"
            },
            {
                -100.0,
                -20.0,
                50.0
            },
            {
                {
                    "A",
                    {-70.0, -10.0, 30.0}
                },
                {
                    "B",
                    {-30.0, -10.0, 20.0}
                }
            },
            100.0,
            100.0
        };
        ql::RiskAttribution attribution;
        const auto result =
            attribution.calculate(riskResult, 0.80);
        expectNear(
            "Basic A component ES",
            result.componentES.at("A"),
            70.0,
            1e-12
        );
        expectNear(
            "Basic B component ES",
            result.componentES.at("B"),
            30.0,
            1e-12
        );
        expectNear(
            "Basic total ES",
            result.totalES,
            100.0,
            1e-12
        );
        expectNear(
            "Basic components sum to total ES",
            result.componentES.at("A") +
                result.componentES.at("B"),
            result.totalES,
            1e-12
        );
    }
    // ============================================================
    // Test 2: Fractional tail at 75% confidence.
//
// Five scenarios give a tail size of 0.25 * 5 = 1.25.
// The worst observation receives weight 1, and the second-worst
// receives weight 0.25.
// ============================================================
    {
        ql::PortfolioHistoricalRiskResult riskResult{
            1000.0,
            0.0,
            {
                "2026-01-02",
                "2026-01-03",
                "2026-01-04",
                "2026-01-05",
                "2026-01-06"
            },
            {
                -100.0,
                -80.0,
                -40.0,
                10.0,
                30.0
            },
            {
                {
                    "A",
                    {-60.0, -50.0, -20.0, 5.0, 10.0}
                },
                {
                    "B",
                    {-40.0, -30.0, -20.0, 5.0, 20.0}
                }
            },
            96.0,
            96.0
        };
        ql::RiskAttribution attribution;
        const auto result =
            attribution.calculate(riskResult, 0.75);
        // Tail size = 1.25
        //
        // A tail loss:
        //   60 + 0.25 * 50 = 72.5
        //
        // ES:
        //   72.5 / 1.25 = 58
        //
        // B:
        //   40 + 0.25 * 30 = 47.5
        //
        // ES:
        //   47.5 / 1.25 = 38
        //
        // Total:
        //   58 + 38 = 96
        expectNear(
            "Fractional A component ES",
            result.componentES.at("A"),
            58.0,
            1e-12
        );
        expectNear(
            "Fractional B component ES",
            result.componentES.at("B"),
            38.0,
            1e-12
        );
        expectNear(
            "Fractional components sum",
            result.componentES.at("A") +
                result.componentES.at("B"),
            96.0,
            1e-12
        );
    }
    // ============================================================
    // Test 3: Hedging instrument
    //
    // Portfolio worst scenario:
    //     Total = -100
    //
    // Instrument A loses:
    //     -130
    //
    // Instrument B gains:
    //     +30
    //
    // Therefore:
    //     A component ES = +130
    //     B component ES = -30
    //
    // The negative component must NOT be clamped to zero.
    //
    // 130 + (-30) = 100
    // ============================================================
    {
        ql::PortfolioHistoricalRiskResult riskResult{
            1000.0,
            0.0,
            {
                "2026-01-02",
                "2026-01-03",
                "2026-01-04"
            },
            {
                -100.0,
                -20.0,
                50.0
            },
            {
                {
                    "A",
                    {-130.0, -10.0, 30.0}
                },
                {
                    "HEDGE",
                    {30.0, -10.0, 20.0}
                }
            },
            100.0,
            100.0
        };
        ql::RiskAttribution attribution;
        const auto result =
            attribution.calculate(riskResult, 0.80);
        expectNear(
            "Hedging instrument A component ES",
            result.componentES.at("A"),
            130.0,
            1e-12
        );
        expectNear(
            "Hedging instrument negative component ES",
            result.componentES.at("HEDGE"),
            -30.0,
            1e-12
        );
        expectNear(
            "Hedging components sum to portfolio ES",
            result.componentES.at("A") +
                result.componentES.at("HEDGE"),
            100.0,
            1e-12
        );
        expectTrue(
            "Hedging component should remain negative",
            result.componentES.at("HEDGE") < 0.0
        );
    }
    // ============================================================
    // Test 4: Risk contribution percentages
    //
    // Portfolio ES = 100
    //
    // A contributes 70 -> 70%
    // B contributes 30 -> 30%
    // ============================================================
    {
        ql::PortfolioHistoricalRiskResult riskResult{
            1000.0,
            0.0,
            {
                "2026-01-02",
                "2026-01-03",
                "2026-01-04"
            },
            {
                -100.0,
                -20.0,
                50.0
            },
            {
                {
                    "A",
                    {-70.0, -10.0, 30.0}
                },
                {
                    "B",
                    {-30.0, -10.0, 20.0}
                }
            },
            100.0,
            100.0
        };
        ql::RiskAttribution attribution;
        const auto result =
            attribution.calculate(riskResult, 0.80);
        expectNear(
            "A contribution percentage",
            result.contributionPercent.at("A"),
            70.0,
            1e-12
        );
        expectNear(
            "B contribution percentage",
            result.contributionPercent.at("B"),
            30.0,
            1e-12
        );
    }
    // ============================================================
    // Test 5: Risk contribution percentages sum to 100%
    // ============================================================
    {
        ql::PortfolioHistoricalRiskResult riskResult{
            1000.0,
            0.0,
            {
                "2026-01-02",
                "2026-01-03",
                "2026-01-04"
            },
            {
                -100.0,
                -20.0,
                50.0
            },
            {
                {
                    "A",
                    {-70.0, -10.0, 30.0}
                },
                {
                    "B",
                    {-30.0, -10.0, 20.0}
                }
            },
            100.0,
            100.0
        };
        ql::RiskAttribution attribution;
        const auto result =
            attribution.calculate(riskResult, 0.80);
        double totalContribution = 0.0;
        for (const auto& [instrumentId, contribution] :
            result.contributionPercent) {
            totalContribution += contribution;
        }
        expectNear(
            "Risk contributions sum to 100 percent",
            totalContribution,
            100.0,
            1e-12
        );
    }
    // ============================================================
    // Test 6: Negative hedge contribution
    //
    // A loses 130
    // HEDGE gains 30
    //
    // Portfolio ES = 100
    //
    // A     = 130%
    // HEDGE = -30%
    // Total = 100%
    // ============================================================
    {
        ql::PortfolioHistoricalRiskResult riskResult{
            1000.0,
            0.0,
            {
                "2026-01-02",
                "2026-01-03",
                "2026-01-04"
            },
            {
                -100.0,
                -20.0,
                50.0
            },
            {
                {
                    "A",
                    {-130.0, -10.0, 30.0}
                },
                {
                    "HEDGE",
                    {30.0, -10.0, 20.0}
                }
            },
            100.0,
            100.0
        };
        ql::RiskAttribution attribution;
        const auto result =
            attribution.calculate(riskResult, 0.80);
        expectNear(
            "A hedge test contribution",
            result.contributionPercent.at("A"),
            130.0,
            1e-12
        );
        expectNear(
            "HEDGE contribution",
            result.contributionPercent.at("HEDGE"),
            -30.0,
            1e-12
        );
        expectNear(
            "Hedge contributions sum to 100 percent",
            result.contributionPercent.at("A") +
                result.contributionPercent.at("HEDGE"),
            100.0,
            1e-12
        );
        expectTrue(
            "HEDGE contribution should be negative",
            result.contributionPercent.at("HEDGE") < 0.0
        );
    }
    // ============================================================
    // Test 7: One long position
    //
    // With only one instrument, its component ES must equal
    // the portfolio ES.
    //
    // Total scenario P&L:
    //     -100, -50, 20
    //
    // At 75% confidence:
    //     tailSize = (1 - 0.75) * 3 = 0.75
    //
    // Only the worst scenario contributes, so the single-position
    // component ES and portfolio ES are both 100.
    // ============================================================
    {
        ql::PortfolioHistoricalRiskResult riskResult{
            1000.0,
            0.0,
            {
                "2026-01-02",
                "2026-01-03",
                "2026-01-04"
            },
            {
                -100.0,
                -50.0,
                20.0
            },
            {
                {
                    "A",
                    {-100.0, -50.0, 20.0}
                }
            },
            100.0,
            100.0
        };
        ql::RiskAttribution attribution;
        const auto result =
            attribution.calculate(riskResult, 0.75);
        expectNear(
            "Single position component ES equals portfolio ES",
            result.componentES.at("A"),
            result.totalES,
            1e-12
        );
        expectNear(
            "Single position component ES",
            result.componentES.at("A"),
            100.0,
            1e-12
        );
        expectNear(
            "Single position contribution percentage",
            result.contributionPercent.at("A"),
            100.0,
            1e-12
        );
    }
    // ============================================================
    // Test 8: Two correlated long positions
    //
    // Both instruments lose money in the same tail scenarios.
    //
    // Total:
    //     -150, -90, 20
    //
    // A:
    //     -90, -50, 10
    //
    // B:
    //     -60, -40, 10
    //
    // At 75% confidence:
    //
    //     tailSize = (1 - 0.75) * 3 = 0.75
    //
    // Only the worst scenario contributes: portfolio ES = 150,
    // A component ES = 90, and B component ES = 60.
    // The components reconcile: 90 + 60 = 150.
    //
    // Both contributions must be positive.
    // ============================================================
    {
        ql::PortfolioHistoricalRiskResult riskResult{
            1000.0,
            0.0,
            {
                "2026-01-02",
                "2026-01-03",
                "2026-01-04"
            },
            {
                -150.0,
                -90.0,
                20.0
            },
            {
                {
                    "A",
                    {-90.0, -50.0, 10.0}
                },
                {
                    "B",
                    {-60.0, -40.0, 10.0}
                }
            },
            150.0,
            150.0
        };
        ql::RiskAttribution attribution;
        const auto result =
            attribution.calculate(riskResult, 0.75);
        expectNear(
            "Correlated A component ES",
            result.componentES.at("A"),
            90.0,
            1e-12
        );
        expectNear(
            "Correlated B component ES",
            result.componentES.at("B"),
            60.0,
            1e-12
        );
        expectTrue(
            "Correlated A contribution should be positive",
            result.componentES.at("A") > 0.0
        );
        expectTrue(
            "Correlated B contribution should be positive",
            result.componentES.at("B") > 0.0
        );
        expectNear(
            "Correlated positions reconcile",
            result.componentES.at("A") +
                result.componentES.at("B"),
            result.totalES,
            1e-12
        );
    }
    // ============================================================
    // Test 9: Hedge
    //
    // A:
    //     -130, -100, 30
    //
    // HEDGE:
    //     +30, +20, -10
    //
    // Total:
    //     -100, -80, 20
    //
    // At 75% confidence:
    //
    //     tailSize = (1 - 0.75) * 3 = 0.75
    //
    // Only the worst scenario contributes: portfolio ES = 100,
    // A component ES = 130, and HEDGE component ES = -30.
    // The components reconcile: 130 + (-30) = 100.
    //
    // The hedge therefore reduces portfolio ES.
    // ============================================================
    {
        ql::PortfolioHistoricalRiskResult riskResult{
            1000.0,
            0.0,
            {
                "2026-01-02",
                "2026-01-03",
                "2026-01-04"
            },
            {
                -100.0,
                -80.0,
                20.0
            },
            {
                {
                    "A",
                    {-130.0, -100.0, 30.0}
                },
                {
                    "HEDGE",
                    {30.0, 20.0, -10.0}
                }
            },
            100.0,
            100.0
        };
        ql::RiskAttribution attribution;
        const auto result =
            attribution.calculate(riskResult, 0.75);
        expectNear(
            "Hedge A component ES",
            result.componentES.at("A"),
            130.0,
            1e-12
        );
        expectNear(
            "Hedge component ES",
            result.componentES.at("HEDGE"),
            -30.0,
            1e-12
        );
        expectTrue(
            "Hedge contribution should be negative",
            result.componentES.at("HEDGE") < 0.0
        );
        expectNear(
            "Hedge reduces portfolio ES",
            result.componentES.at("A") +
                result.componentES.at("HEDGE"),
            100.0,
            1e-12
        );
        expectNear(
            "Hedge contribution percentage",
            result.contributionPercent.at("HEDGE"),
            -30.0 / 100.0 * 100.0,
            1e-12
        );
    }
    // ============================================================
    // Test 10: Multiple lots of one instrument
    //
    // Two lots of instrument A:
    //
    // Lot 1 scenario P&L:
    //     -60, -30, 10
    //
    // Lot 2 scenario P&L:
    //     -40, -20, 10
    //
    // Aggregated A scenario P&L:
    //     -100, -50, 20
    //
    // RiskAttribution should attribute the combined result to
    // instrument A.
    //
    // At 75% confidence:
    //
    //     tailSize = (1 - 0.75) * 3 = 0.75
    //
    // Only the worst scenario contributes, so component ES = 100.
    // ============================================================
    {
        // This is the already-aggregated result that
        // PortfolioHistoricalRisk produces for multiple lots
        // having the same instrument ID.
        ql::PortfolioHistoricalRiskResult riskResult{
            1000.0,
            0.0,
            {
                "2026-01-02",
                "2026-01-03",
                "2026-01-04"
            },
            {
                -100.0,
                -50.0,
                20.0
            },
            {
                {
                    "A",
                    {-100.0, -50.0, 20.0}
                }
            },
            100.0,
            100.0
        };
        ql::RiskAttribution attribution;
        const auto result =
            attribution.calculate(riskResult, 0.75);
        expectNear(
            "Multiple lots aggregate into one instrument ES",
            result.componentES.at("A"),
            100.0,
            1e-12
        );
        expectNear(
            "Multiple lots reconcile with portfolio ES",
            result.componentES.at("A"),
            result.totalES,
            1e-12
        );
        expectNear(
            "Multiple lots contribution percentage",
            result.contributionPercent.at("A"),
            100.0,
            1e-12
        );
        expectTrue(
            "Multiple lots produce one instrument component",
            result.componentES.size() == 1
        );
    }
    // ============================================================
    // Test 11: Zero portfolio ES when all scenarios are profitable
    //
    // The portfolio risk engine clamps Expected Shortfall to zero.
    // Attribution must return zero component ES and zero contribution
    // percentages instead of failing reconciliation.
    // ============================================================
    {
        ql::PortfolioHistoricalRiskResult riskResult{
            1000.0,
            0.0,
            {
                "2026-01-02",
                "2026-01-03",
                "2026-01-04"
            },
            {
                10.0,
                20.0,
                30.0
            },
            {
                {
                    "A",
                    {5.0, 10.0, 15.0}
                },
                {
                    "B",
                    {5.0, 10.0, 15.0}
                }
            },
            0.0,
            0.0
        };
        ql::RiskAttribution attribution;
        const auto result = attribution.calculate(riskResult, 0.80);
        expectNear("Zero ES total", result.totalES, 0.0, 1e-12);
        expectNear("Zero ES component A", result.componentES.at("A"), 0.0, 1e-12);
        expectNear("Zero ES component B", result.componentES.at("B"), 0.0, 1e-12);
        expectNear("Zero ES contribution A", result.contributionPercent.at("A"), 0.0, 1e-12);
        expectNear("Zero ES contribution B", result.contributionPercent.at("B"), 0.0, 1e-12);
        expectTrue(
            "Zero ES returns one component per instrument",
            result.componentES.size() == 2 &&
            result.contributionPercent.size() == 2
        );
    }
    // ============================================================
    // Test 12: Reject mismatched P&Ls in a non-tail scenario
    //
    // At 80% confidence with five scenarios, only the worst
    // scenario is in the ES tail. Scenario index 4 is profitable
    // and outside the tail, but its instrument P&Ls do not sum
    // to the portfolio P&L. The input must still be rejected.
    // ============================================================
    {
        ql::PortfolioHistoricalRiskResult riskResult{
            1000.0,
            0.0,
            {
                "2026-01-02",
                "2026-01-03",
                "2026-01-04",
                "2026-01-05",
                "2026-01-06"
            },
            {
                -100.0,
                -20.0,
                0.0,
                20.0,
                40.0
            },
            {
                {
                    "A",
                    {-70.0, -10.0, 0.0, 10.0, 20.0}
                },
                {
                    "B",
                    {-30.0, -10.0, 0.0, 10.0, 25.0}
                }
            },
            100.0,
            100.0
        };
        ql::RiskAttribution attribution;
        expectInvalidArgument(
            "Non-tail scenario P&L mismatch is rejected",
            [&] { (void)attribution.calculate(riskResult, 0.80); },
            "scenario index 4"
        );
    }
    // Test 13: Reject results without instrument-level scenario P&L.
    {
        ql::PortfolioHistoricalRiskResult riskResult{
            1000.0,
            0.0,
            {"2026-01-02", "2026-01-03"},
            {-100.0, 50.0},
            {},
            100.0,
            100.0
        };
        ql::RiskAttribution attribution;
        expectInvalidArgument(
            "Missing instrument scenario P&Ls are rejected",
            [&] { (void)attribution.calculate(riskResult, 0.50); }
        );
    }
    return 0;
}
catch (const std::exception& e) {
    std::cerr << "TEST EXCEPTION: "
              << e.what()
              << '\n';
    return 1;
}
}
