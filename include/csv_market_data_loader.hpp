
#pragma once

#include "portfolio_historical_risk.hpp"

#include <string>
#include <vector>

namespace ql {

class CsvMarketDataLoader {
public:
    // Expected CSV columns: instrumentId,date,price.
    // Dates must be valid YYYY-MM-DD dates, strictly increasing
    // for each instrument.
    //
    // Throws std::invalid_argument for malformed data and
    // std::runtime_error if the file cannot be opened or read.
    static std::vector<HistoricalPriceSeries> loadPriceSeries(
        const std::string& filePath
    );
};

} // namespace ql
