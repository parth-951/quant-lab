
#include "csv_market_data_loader.hpp"

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void expectTrue(const std::string& name, bool condition) {
    if (!condition) {
        std::cerr << "FAILED: " << name << '\n';
        std::exit(1);
    }
}

template <typename Exception, typename Function>
void expectThrows(const std::string& name, Function function) {
    try {
        function();
    } catch (const Exception&) {
        return;
    } catch (...) {
        std::cerr << "FAILED: " << name
                  << " (wrong exception type)\n";
        std::exit(1);
    }

    std::cerr << "FAILED: " << name
              << " (no exception thrown)\n";
    std::exit(1);
}

std::filesystem::path writeCsv(const std::string& contents) {
    const auto path =
        std::filesystem::temp_directory_path() /
        "quant_lab_market_data_loader_test.csv";

    std::ofstream output(path);
    if (!output) {
        throw std::runtime_error("Could not create test CSV.");
    }

    output << contents;
    output.close();

    return path;
}

} // namespace

int main() {
    // Valid input: two instruments with chronological price histories.
    {
        const auto path = writeCsv(
            "instrumentId,date,price\n"
            "AAPL,2026-01-02,243.85\n"
            "MSFT,2026-01-02,417.25\n"
            "AAPL,2026-01-05,246.10\n"
            "MSFT,2026-01-05,420.30\n"
        );

        const auto histories =
            ql::CsvMarketDataLoader::loadPriceSeries(path.string());

        expectTrue("Two instruments loaded", histories.size() == 2);
        expectTrue("First instrument ID",
                   histories[0].instrumentId == "AAPL");
        expectTrue("Second instrument ID",
                   histories[1].instrumentId == "MSFT");
        expectTrue("AAPL dates loaded",
                   histories[0].dates.size() == 2);
        expectTrue("AAPL dates are chronological",
                   histories[0].dates[0] == "2026-01-02" &&
                   histories[0].dates[1] == "2026-01-05");
        expectTrue("AAPL first price",
                   std::abs(histories[0].prices[0] - 243.85) < 1e-10);
        expectTrue("MSFT second price",
                   std::abs(histories[1].prices[1] - 420.30) < 1e-10);

        std::filesystem::remove(path);
    }

    // Reject invalid calendar dates.
    {
        const auto path = writeCsv(
            "instrumentId,date,price\n"
            "AAPL,2026-02-30,100\n"
        );

        expectThrows<std::invalid_argument>(
            "Invalid calendar date rejected",
            [&] {
                (void)ql::CsvMarketDataLoader::loadPriceSeries(
                    path.string()
                );
            }
        );

        std::filesystem::remove(path);
    }

    // Reject duplicate dates for an instrument.
    {
        const auto path = writeCsv(
            "instrumentId,date,price\n"
            "AAPL,2026-01-02,100\n"
            "AAPL,2026-01-02,101\n"
        );

        expectThrows<std::invalid_argument>(
            "Duplicate date rejected",
            [&] {
                (void)ql::CsvMarketDataLoader::loadPriceSeries(
                    path.string()
                );
            }
        );

        std::filesystem::remove(path);
    }

    // Reject a non-positive price.
    {
        const auto path = writeCsv(
            "instrumentId,date,price\n"
            "AAPL,2026-01-02,0\n"
        );

        expectThrows<std::invalid_argument>(
            "Zero price rejected",
            [&] {
                (void)ql::CsvMarketDataLoader::loadPriceSeries(
                    path.string()
                );
            }
        );

        std::filesystem::remove(path);
    }

    // Reject malformed rows.
    {
        const auto path = writeCsv(
            "instrumentId,date,price\n"
            "AAPL,2026-01-02\n"
        );

        expectThrows<std::invalid_argument>(
            "Missing CSV field rejected",
            [&] {
                (void)ql::CsvMarketDataLoader::loadPriceSeries(
                    path.string()
                );
            }
        );

        std::filesystem::remove(path);
    }

    std::cout << "CSV market-data loader tests passed.\n";
    return 0;
}
