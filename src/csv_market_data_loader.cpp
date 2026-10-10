
#include "csv_market_data_loader.hpp"

#include <cmath>
#include <cctype>
#include <fstream>
#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace ql {
namespace {

std::string trim(const std::string& value) {
    std::size_t first = 0;
    while (first < value.size() &&
           std::isspace(static_cast<unsigned char>(value[first]))) {
        ++first;
    }

    std::size_t last = value.size();
    while (last > first &&
           std::isspace(static_cast<unsigned char>(value[last - 1]))) {
        --last;
    }

    return value.substr(first, last - first);
}

std::vector<std::string> parseCsvRow(const std::string& line) {
    std::vector<std::string> fields;
    std::string field;
    bool inQuotes = false;
    bool closedQuote = false;

    for (std::size_t i = 0; i < line.size(); ++i) {
        const char ch = line[i];

        if (inQuotes) {
            if (ch == '"') {
                if (i + 1 < line.size() && line[i + 1] == '"') {
                    field += '"';
                    ++i;
                } else {
                    inQuotes = false;
                    closedQuote = true;
                }
            } else {
                field += ch;
            }
        } else if (ch == ',' ) {
            fields.push_back(trim(field));
            field.clear();
            closedQuote = false;
        } else if (ch == '"' && trim(field).empty() && !closedQuote) {
            field.clear();
            inQuotes = true;
        } else {
            if (closedQuote && !std::isspace(
                    static_cast<unsigned char>(ch))) {
                throw std::invalid_argument(
                    "Unexpected character after a quoted CSV field."
                );
            }
            if (!closedQuote) {
                field += ch;
            }
        }
    }

    if (inQuotes) {
        throw std::invalid_argument("Unclosed quoted CSV field.");
    }

    fields.push_back(trim(field));
    return fields;
}

bool isLeapYear(int year) {
    return year % 400 == 0 ||
           (year % 4 == 0 && year % 100 != 0);
}

bool isValidIsoDate(const std::string& date) {
    if (date.size() != 10 ||
        date[4] != '-' ||
        date[7] != '-') {
        return false;
    }

    for (std::size_t i = 0; i < date.size(); ++i) {
        if (i == 4 || i == 7) {
            continue;
        }
        if (!std::isdigit(static_cast<unsigned char>(date[i]))) {
            return false;
        }
    }

    const int year = std::stoi(date.substr(0, 4));
    const int month = std::stoi(date.substr(5, 2));
    const int day = std::stoi(date.substr(8, 2));

    if (year < 1 || month < 1 || month > 12 || day < 1) {
        return false;
    }

    const int daysInMonth[] = {
        31, isLeapYear(year) ? 29 : 28, 31, 30, 31, 30,
        31, 31, 30, 31, 30, 31
    };

    return day <= daysInMonth[month - 1];
}

double parsePrice(const std::string& text) {
    if (text.empty()) {
        throw std::invalid_argument("Price field is empty.");
    }

    std::size_t parsed = 0;
    double price = 0.0;

    try {
        price = std::stod(text, &parsed);
    } catch (const std::exception&) {
        throw std::invalid_argument("Price is not a valid number.");
    }

    if (parsed != text.size()) {
        throw std::invalid_argument(
            "Price contains unexpected trailing characters."
        );
    }

    if (!std::isfinite(price) || price <= 0.0) {
        throw std::invalid_argument(
            "Price must be positive and finite."
        );
    }

    return price;
}

} // namespace

std::vector<HistoricalPriceSeries> CsvMarketDataLoader::loadPriceSeries(
    const std::string& filePath
) {
    std::ifstream input(filePath);

    if (!input.is_open()) {
        throw std::runtime_error(
            "Could not open market-data CSV: " + filePath
        );
    }

    std::string line;
    if (!std::getline(input, line)) {
        throw std::invalid_argument("Market-data CSV is empty.");
    }

    // Remove an optional UTF-8 BOM.
    if (line.size() >= 3 &&
        static_cast<unsigned char>(line[0]) == 0xEF &&
        static_cast<unsigned char>(line[1]) == 0xBB &&
        static_cast<unsigned char>(line[2]) == 0xBF) {
        line.erase(0, 3);
    }

    const auto header = parseCsvRow(line);
    if (header.size() != 3 ||
        header[0] != "instrumentId" ||
        header[1] != "date" ||
        header[2] != "price") {
        throw std::invalid_argument(
            "CSV header must be: instrumentId,date,price."
        );
    }

    std::vector<HistoricalPriceSeries> result;
    std::map<std::string, std::size_t> seriesIndex;
    std::size_t lineNumber = 1;

    while (std::getline(input, line)) {
        ++lineNumber;

        if (trim(line).empty()) {
            continue;
        }

        try {
            const auto fields = parseCsvRow(line);
            if (fields.size() != 3) {
                throw std::invalid_argument(
                    "Each data row must contain exactly three fields."
                );
            }

            const std::string instrumentId = fields[0];
            const std::string date = fields[1];

            if (instrumentId.empty()) {
                throw std::invalid_argument(
                    "Instrument ID must not be empty."
                );
            }

            if (!isValidIsoDate(date)) {
                throw std::invalid_argument(
                    "Date must be a valid YYYY-MM-DD calendar date."
                );
            }

            const double price = parsePrice(fields[2]);

            auto it = seriesIndex.find(instrumentId);
            if (it == seriesIndex.end()) {
                const std::size_t index = result.size();
                seriesIndex.emplace(instrumentId, index);
                result.push_back(
                    HistoricalPriceSeries{instrumentId, {}, {}}
                );
                it = seriesIndex.find(instrumentId);
            }

            auto& series = result[it->second];

            if (!series.dates.empty() &&
                date <= series.dates.back()) {
                throw std::invalid_argument(
                    "Dates must be strictly increasing for each instrument."
                );
            }

            series.dates.push_back(date);
            series.prices.push_back(price);
        } catch (const std::exception& e) {
            throw std::invalid_argument(
                "CSV line " + std::to_string(lineNumber) +
                ": " + e.what()
            );
        }
    }

    if (input.bad()) {
        throw std::runtime_error(
            "Error while reading market-data CSV: " + filePath
        );
    }

    if (result.empty()) {
        throw std::invalid_argument(
            "Market-data CSV contains no price observations."
        );
    }

    return result;
}

} // namespace ql
