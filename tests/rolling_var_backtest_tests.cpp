
#include "rolling_var_backtest.hpp"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

bool near(
    double actual,
    double expected,
    double tolerance = 1e-10) {
    return std::isfinite(actual) &&
           std::abs(actual - expected) <= tolerance;
}

template <typename Function>
void requireThrows(
    Function&& function,
    const std::string& message) {
    bool threw = false;

    try {
        function();
    } catch (const std::invalid_argument&) {
        threw = true;
    }

    require(threw, message);
}

} // namespace

int main() {
    try {
        const std::vector<double> returns{
            -0.10, 0.10, -0.20, 0.00
        };

        // 1. Existing fixed-notional behavior.
        const auto fixed = ql::RollingVaRBacktest::run(
            returns, 2, 100.0, 0.75);

        require(
            fixed.predictedVaR.size() == 2,
            "Expected two fixed-notional forecasts");

        require(
            fixed.realizedLosses.size() == 2,
            "Expected two fixed-notional realized losses");

        require(
            fixed.forecastIndices ==
                std::vector<std::size_t>{2, 3},
            "Forecast indices should identify the tested days");

        require(
            near(fixed.predictedVaR[0], 5.0),
            "First fixed-notional forecast mismatch");

        require(
            near(fixed.predictedVaR[1], 12.5),
            "Second fixed-notional forecast mismatch");

        require(
            near(fixed.realizedLosses[0], 20.0),
            "First fixed-notional realized loss mismatch");

        require(
            near(fixed.realizedLosses[1], 0.0),
            "Zero return should produce zero loss");

        require(
            fixed.backtest.observations == 2 &&
            fixed.backtest.breaches == 1,
            "Fixed-notional backtest results mismatch");

        // 2. Compounded mode reflects returns before the forecast date.
        const auto compounded = ql::RollingVaRBacktest::run(
            returns,
            2,
            100.0,
            0.75,
            0.05,
            ql::PortfolioValueMode::Compounded);

        // Start of index 2:
        // 100 * (1 - 0.10) * (1 + 0.10) = 99.
        require(
            near(compounded.predictedVaR[0], 4.95),
            "First compounded forecast should use value 99");

        require(
            near(compounded.realizedLosses[0], 19.8),
            "First compounded loss should use value 99");

        // After the -20% return at index 2:
        // 99 * 0.8 = 79.2 at the start of index 3.
        require(
            near(compounded.predictedVaR[1], 9.9),
            "Second compounded forecast should use value 79.2");

        require(
            near(compounded.realizedLosses[1], 0.0),
            "Zero return should produce zero compounded loss");

        // 3. No look-ahead: changing return[t] cannot change VaR[t].
        std::vector<double> altered = returns;
        altered[2] = 0.50;

        const auto changedCurrentReturn = ql::RollingVaRBacktest::run(
            altered,
            2,
            100.0,
            0.75,
            0.05,
            ql::PortfolioValueMode::FixedNotional);

        require(
            near(changedCurrentReturn.predictedVaR[0],
                 fixed.predictedVaR[0]),
            "Current-day return must not affect its own forecast");

        require(
            !near(changedCurrentReturn.realizedLosses[0],
                  fixed.realizedLosses[0]),
            "Changing current-day return should change realized loss");

        // 4. Actual P&L uses conventional signs:
        // profits are positive, losses are negative.
        const auto pnlResult = ql::RollingVaRBacktest::runPnl(
            {10.0, 20.0, 15.0},
            {-12.0, 5.0, -30.0},
            0.95);

        require(
            pnlResult.predictedVaR ==
                std::vector<double>{10.0, 20.0, 15.0},
            "P&L mode should preserve supplied VaR forecasts");

        require(
            pnlResult.realizedLosses ==
                std::vector<double>{12.0, 0.0, 30.0},
            "P&L mode should convert P&L to nonnegative losses");

        require(
            pnlResult.forecastIndices ==
                std::vector<std::size_t>{0, 1, 2},
            "P&L forecast indices mismatch");

        // 5. Mismatched P&L and VaR lengths must be rejected.
        requireThrows([] {
            (void)ql::RollingVaRBacktest::runPnl(
                {10.0, 20.0},
                {-12.0},
                0.95);
        }, "Mismatched P&L vector lengths should throw");

        // 6. Empty P&L inputs must be rejected.
        requireThrows([] {
            (void)ql::RollingVaRBacktest::runPnl(
                {}, {}, 0.95);
        }, "Empty P&L inputs should throw");

        // 7. Non-finite P&L must be rejected.
        requireThrows([] {
            (void)ql::RollingVaRBacktest::runPnl(
                {10.0},
                {std::numeric_limits<double>::quiet_NaN()},
                0.95);
        }, "Non-finite realized P&L should throw");

        // 8. Negative VaR forecast must be rejected.
        requireThrows([] {
            (void)ql::RollingVaRBacktest::runPnl(
                {-1.0}, {-2.0}, 0.95);
        }, "Negative predicted VaR should throw");

        // 9. Compounded mode accepts a total loss of 100%.
        const auto zeroValue = ql::RollingVaRBacktest::run(
            {-1.0, 0.10},
            1,
            100.0,
            0.75,
            0.05,
            ql::PortfolioValueMode::Compounded);

        // The -100% return occurs before forecast index 1.
        // Portfolio value at the start of index 1 is zero.
        require(
            near(zeroValue.predictedVaR[0], 0.0),
            "VaR should be zero after portfolio value reaches zero");

        require(
            near(zeroValue.realizedLosses[0], 0.0),
            "Loss should be zero after portfolio value reaches zero");

        // 10. Fixed-notional mode retains its original return constraint.
        requireThrows([] {
            (void)ql::RollingVaRBacktest::run(
                {-1.0, 0.10},
                1,
                100.0,
                0.75);
        }, "Fixed-notional mode should reject a -100% return");

        // 11. Existing validation.
        requireThrows([] {
            (void)ql::RollingVaRBacktest::run(
                {-0.01, 0.02}, 0, 100.0, 0.95);
        }, "Zero lookback should be rejected");

        requireThrows([] {
            (void)ql::RollingVaRBacktest::run(
                {-0.01, 0.02}, 2, 100.0, 0.95);
        }, "Insufficient returns should be rejected");

        requireThrows([] {
            (void)ql::RollingVaRBacktest::run(
                {-0.01, 0.02, 0.03}, 2, -100.0, 0.95);
        }, "Negative portfolio value should be rejected");

        requireThrows([] {
            (void)ql::RollingVaRBacktest::run(
                {-0.01, 0.02, 0.03}, 2, 100.0, 1.0);
        }, "Invalid confidence level should be rejected");

        requireThrows([] {
            (void)ql::RollingVaRBacktest::run(
                {-0.01, 0.02, 0.03}, 2, 100.0, 0.95, 0.0);
        }, "Invalid significance level should be rejected");

        requireThrows([] {
            (void)ql::RollingVaRBacktest::run(
                {-0.01, 0.02,
                 std::numeric_limits<double>::quiet_NaN()},
                2, 100.0, 0.95);
        }, "Non-finite return should be rejected");
        // 12. Portfolio P&L backtest supports changing portfolio values.
        const auto portfolioPnl = ql::RollingVaRBacktest::runPortfolioPnl(
            {10.0, 20.0, 15.0, 25.0},
            {-12.0, 5.0, -30.0, -26.0},
            {100.0, 150.0, 120.0, 200.0},
            0.95);

        require(
            portfolioPnl.predictedVaR ==
                std::vector<double>{10.0, 20.0, 15.0, 25.0},
            "Portfolio P&L mode should preserve supplied VaR forecasts");

        require(
            portfolioPnl.realizedLosses ==
                std::vector<double>{12.0, 0.0, 30.0, 26.0},
            "Portfolio P&L mode should convert P&L to nonnegative losses");

        require(
            portfolioPnl.forecastIndices ==
                std::vector<std::size_t>{0, 1, 2, 3},
            "Portfolio P&L mode should preserve daily alignment");

        require(
            portfolioPnl.backtest.observations == 4 &&
            portfolioPnl.backtest.breaches == 3,
            "Portfolio P&L breach count mismatch");

        // 13. Portfolio values are validated even though forecasts
        // and P&L are supplied by the caller.
        requireThrows([] {
            (void)ql::RollingVaRBacktest::runPortfolioPnl(
                {10.0, 20.0},
                {-12.0, 5.0},
                {100.0},
                0.95);
        }, "Mismatched portfolio value lengths should throw");

        requireThrows([] {
            (void)ql::RollingVaRBacktest::runPortfolioPnl(
                {10.0},
                {-12.0},
                {-100.0},
                0.95);
        }, "Negative portfolio value should throw");

        requireThrows([] {
            (void)ql::RollingVaRBacktest::runPortfolioPnl(
                {10.0},
                {-12.0},
                {std::numeric_limits<double>::infinity()},
                0.95);
        }, "Non-finite portfolio value should throw");

        // 14. Portfolio P&L backtest validates forecasts and outcomes.
        requireThrows([] {
            (void)ql::RollingVaRBacktest::runPortfolioPnl(
                {std::numeric_limits<double>::quiet_NaN()},
                {-12.0},
                {100.0},
                0.95);
        }, "Non-finite VaR should throw");

        requireThrows([] {
            (void)ql::RollingVaRBacktest::runPortfolioPnl(
                {-1.0},
                {-12.0},
                {100.0},
                0.95);
        }, "Negative VaR should throw");

        requireThrows([] {
            (void)ql::RollingVaRBacktest::runPortfolioPnl(
                {10.0},
                {std::numeric_limits<double>::infinity()},
                {100.0},
                0.95);
        }, "Non-finite portfolio P&L should throw");

        requireThrows([] {
            (void)ql::RollingVaRBacktest::runPortfolioPnl(
                {}, {}, {}, 0.95);
        }, "Empty portfolio P&L inputs should throw");
        std::cout << "All rolling VaR backtest tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Rolling VaR backtest test failure: "
                  << error.what() << '\n';
        return 1;
    }
}
