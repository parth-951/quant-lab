#include "portfolio_historical_risk.hpp"

#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
#include <cctype>
#include <chrono>

namespace ql {

namespace {
void validateCommonDateGrid(
    const std::vector<std::map<std::string, double>>& priceMaps
) {
    if (priceMaps.empty()) {
        throw std::invalid_argument(
            "At least one portfolio historical price series is required"
        );
    }

    const auto& reference = priceMaps.front();

    for (std::size_t i = 1; i < priceMaps.size(); ++i) {
        if (priceMaps[i].size() != reference.size()) {
            throw std::invalid_argument(
                "All portfolio historical price series must share the same date grid"
            );
        }

        auto referenceIt = reference.begin();
        auto currentIt = priceMaps[i].begin();

        while (referenceIt != reference.end()) {
            if (referenceIt->first != currentIt->first) {
                throw std::invalid_argument(
                    "All portfolio historical price series must share the same date grid"
                );
            }

            ++referenceIt;
            ++currentIt;
        }
    }
}
void validateConfidenceLevel(double confidenceLevel) {
    if (!std::isfinite(confidenceLevel) ||
        confidenceLevel <= 0.0 ||
        confidenceLevel >= 1.0) {
        throw std::invalid_argument(
            "Confidence level must be between 0 and 1"
        );
    }
}
bool isLeapYear(int year) {
    return (year % 400 == 0) ||
           (year % 4 == 0 && year % 100 != 0);
}

bool isValidIsoDate(const std::string& date) {
    if (date.size() != 10) {
        return false;
    }

    if (date[4] != '-' || date[7] != '-') {
        return false;
    }

    for (std::size_t i = 0; i < date.size(); ++i) {
        if (i == 4 || i == 7) {
            continue;
        }

        if (!std::isdigit(
                static_cast<unsigned char>(date[i]))) {
            return false;
        }
    }

    const int year = std::stoi(date.substr(0, 4));
    const int month = std::stoi(date.substr(5, 2));
    const int day = std::stoi(date.substr(8, 2));

    if (year < 1) {
        return false;
    }

    if (month < 1 || month > 12) {
        return false;
    }

    int daysInMonth;

    switch (month) {
        case 2:
            daysInMonth = isLeapYear(year) ? 29 : 28;
            break;

        case 4:
        case 6:
        case 9:
        case 11:
            daysInMonth = 30;
            break;

        default:
            daysInMonth = 31;
            break;
    }

    return day >= 1 && day <= daysInMonth;
}

void validateHistory(const HistoricalPriceSeries& history) {
    if (history.instrumentId.empty()) {
        throw std::invalid_argument(
            "Historical price series instrument ID must not be empty"
        );
    }

    if (history.dates.size() != history.prices.size()) {
        throw std::invalid_argument(
            "Historical dates and prices must have the same size"
        );
    }

    if (history.prices.size() < 2) {
        throw std::invalid_argument(
            "Each historical price series must contain at least two prices"
        );
    }

    std::set<std::string> uniqueDates;

    for (std::size_t i = 0; i < history.dates.size(); ++i) {
        if (!isValidIsoDate(history.dates[i])) {
        throw std::invalid_argument(
            "Historical date must use YYYY-MM-DD format"
        );
    }

        if (!uniqueDates.insert(history.dates[i]).second) {
            throw std::invalid_argument(
                "Historical price series contains duplicate dates"
            );
        }

        if (!std::isfinite(history.prices[i]) ||
            history.prices[i] <= 0.0) {
            throw std::invalid_argument(
                "Historical prices must be positive and finite"
            );
        }
    }
}

std::map<std::string, double> makePriceMap(
    const HistoricalPriceSeries& history
) {
    std::map<std::string, double> result;

    for (std::size_t i = 0; i < history.dates.size(); ++i) {
        result.emplace(
            history.dates[i],
            history.prices[i]
        );
    }

    return result;
}

std::vector<std::string> findCommonDates(
    const std::vector<std::map<std::string, double>>& priceMaps
) {
    if (priceMaps.empty()) {
        throw std::invalid_argument(
            "At least one historical price series is required"
        );
    }

    std::vector<std::string> commonDates;

    for (const auto& entry : priceMaps[0]) {
        const std::string& date = entry.first;

        bool presentInAll = true;

        for (std::size_t i = 1; i < priceMaps.size(); ++i) {
            if (priceMaps[i].find(date) == priceMaps[i].end()) {
                presentInAll = false;
                break;
            }
        }

        if (presentInAll) {
            commonDates.push_back(date);
        }
    }

    if (commonDates.size() < 2) {
        throw std::invalid_argument(
            "At least two common historical dates are required"
        );
    }

    return commonDates;
}

double calculateHistoricalVaR(
    std::vector<double> scenarioPnl,
    double confidenceLevel
) {
    std::sort(
        scenarioPnl.begin(),
        scenarioPnl.end()
    );

    const double position =
        (1.0 - confidenceLevel) *
        static_cast<double>(scenarioPnl.size() - 1);

    const std::size_t lowerIndex =
        static_cast<std::size_t>(std::floor(position));

    const std::size_t upperIndex =
        static_cast<std::size_t>(std::ceil(position));

    const double fraction =
        position - static_cast<double>(lowerIndex);

    const double quantile =
        scenarioPnl[lowerIndex] +
        fraction *
            (scenarioPnl[upperIndex] -
             scenarioPnl[lowerIndex]);

    return std::max(0.0, -quantile);
}

double calculateExpectedShortfall(
    std::vector<double> scenarioPnl,
    double confidenceLevel
) {
    std::sort(
        scenarioPnl.begin(),
        scenarioPnl.end()
    );

    const double tailSize =
        (1.0 - confidenceLevel) *
        static_cast<double>(scenarioPnl.size());

    if (tailSize <= 0.0) {
        throw std::invalid_argument(
            "Expected Shortfall tail must contain observations"
        );
    }

    const std::size_t fullTailCount =
        static_cast<std::size_t>(
            std::floor(tailSize)
        );

    double tailLoss = 0.0;

    for (std::size_t i = 0; i < fullTailCount; ++i) {
        tailLoss += -scenarioPnl[i];
    }

    const double fractionalPart =
        tailSize -
        static_cast<double>(fullTailCount);

    if (fractionalPart > 0.0 &&
        fullTailCount < scenarioPnl.size()) {
        tailLoss +=
            fractionalPart *
            (-scenarioPnl[fullTailCount]);
    }

    const double expectedShortfall =
        tailLoss / tailSize;

    return std::max(0.0, expectedShortfall);
}

} // namespace

PortfolioHistoricalRiskResult PortfolioHistoricalRisk::calculate(
    const Portfolio& portfolio,
    const std::vector<HistoricalPriceSeries>& histories,
    double confidenceLevel
) const {
    validateConfidenceLevel(confidenceLevel);

    if (portfolio.positions().empty()) {
        throw std::invalid_argument(
            "Portfolio must contain at least one position"
        );
    }

    if (histories.empty()) {
        throw std::invalid_argument(
            "At least one historical price series is required"
        );
    }

    // ------------------------------------------------------------
    // 1. Validate histories and create price maps.
    // ------------------------------------------------------------

    std::map<std::string, HistoricalPriceSeries> historyByInstrument;

    for (const HistoricalPriceSeries& history : histories) {
        validateHistory(history);

        if (historyByInstrument.find(history.instrumentId) !=
            historyByInstrument.end()) {
            throw std::invalid_argument(
                "Duplicate historical price series instrument ID"
            );
        }

        historyByInstrument.insert(
            {history.instrumentId, history}
        );
    }

    // ------------------------------------------------------------
    // 2. Every portfolio position must have historical prices.
    // ------------------------------------------------------------

    for (const Position& position : portfolio.positions()) {
        if (historyByInstrument.find(position.instrumentId) ==
            historyByInstrument.end()) {
            throw std::invalid_argument(
                "Missing historical price series for portfolio position: " +
                position.instrumentId
            );
        }
    }

    // ------------------------------------------------------------
    // 3. Build and store price maps ONLY for instruments held
    //    in the portfolio.
    // ------------------------------------------------------------

    std::map<std::string, std::map<std::string, double>>
        priceMapsByInstrument;

    for (const Position& position : portfolio.positions()) {
        const HistoricalPriceSeries& history =
            historyByInstrument.at(position.instrumentId);

        if (priceMapsByInstrument.find(position.instrumentId) ==
            priceMapsByInstrument.end()) {
            priceMapsByInstrument.insert(
                {
                    position.instrumentId,
                    makePriceMap(history)
                }
            );
        }
    }

    // ------------------------------------------------------------
    // 4. Find dates shared only by portfolio instruments.
    // ------------------------------------------------------------

    std::vector<std::map<std::string, double>> portfolioPriceMaps;

    for (const Position& position : portfolio.positions()) {
        portfolioPriceMaps.push_back(
            priceMapsByInstrument.at(position.instrumentId)
        );
    }

    validateCommonDateGrid(portfolioPriceMaps);

    const std::vector<std::string> commonDates =
        findCommonDates(portfolioPriceMaps);

    // ------------------------------------------------------------
    // 5. Build scenario P&L.
    // ------------------------------------------------------------

    

    
    std::vector<double> scenarioPnl;
    scenarioPnl.reserve(commonDates.size() - 1);

    std::map<std::string, std::vector<double>> instrumentScenarioPnl;

    for (const Position& position : portfolio.positions()) {
        instrumentScenarioPnl[position.instrumentId] = {};
        instrumentScenarioPnl[position.instrumentId].reserve(
            commonDates.size() - 1
        );
    }

    for (std::size_t dateIndex = 1;
        dateIndex < commonDates.size();
        ++dateIndex) {

        const std::string& previousDate =
            commonDates[dateIndex - 1];

        const std::string& currentDate =
            commonDates[dateIndex];

        double totalScenarioPnl = 0.0;

        std::map<std::string, double> scenarioInstrumentPnl;

        for (const Position& position : portfolio.positions()) {
            const std::map<std::string, double>& priceMap =
                priceMapsByInstrument.at(position.instrumentId);

            const double previousPrice =
                priceMap.at(previousDate);

            const double currentHistoricalPrice =
                priceMap.at(currentDate);

            const double historicalReturn =
                (currentHistoricalPrice - previousPrice) /
                previousPrice;

            const double positionScenarioPnl =
                position.quantity *
                position.currentPrice *
                historicalReturn;

            totalScenarioPnl += positionScenarioPnl;

            scenarioInstrumentPnl[position.instrumentId] +=
                positionScenarioPnl;
        }

        scenarioPnl.push_back(totalScenarioPnl);

        for (const auto& [instrumentId, instrumentPnl] :
            scenarioInstrumentPnl) {

            instrumentScenarioPnl[instrumentId].push_back(
                instrumentPnl
            );
        }
    }

    // ------------------------------------------------------------
    // 6. Scenario dates correspond to return ending dates.
    // ------------------------------------------------------------

    std::vector<std::string> scenarioDates(
        commonDates.begin() + 1,
        commonDates.end()
    );

    // ------------------------------------------------------------
    // 7. Calculate currency VaR and Expected Shortfall.
    // ------------------------------------------------------------

    const double var =
        calculateHistoricalVaR(
            scenarioPnl,
            confidenceLevel
        );

    const double expectedShortfall =
        calculateExpectedShortfall(
            scenarioPnl,
            confidenceLevel
        );

    // ------------------------------------------------------------
    // 8. Return complete portfolio risk result.
    // ------------------------------------------------------------

    return {
    portfolio.marketValue(),
    portfolio.pnl(),
    scenarioDates,
    scenarioPnl,
    instrumentScenarioPnl,
    var,
    expectedShortfall
};
}
} // namespace ql