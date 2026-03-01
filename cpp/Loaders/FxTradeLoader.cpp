#include "FxTradeLoader.h"

#include <stdexcept>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {
    const std::string seperator = u8"¬";

    std::vector<std::string> splitByString(const std::string& s, const std::string& seperator) {
        std::vector<std::string> out;
        size_t start = 0;
        while (true) {
            size_t position = s.find(seperator, start);
            if (position == std::string::npos) {
                out.push_back(s.substr(start));
                break;
            }
            out.push_back(s.substr(start, position - start));
            start = position + seperator.size();
        }
        return out;
    }

    std::chrono::system_clock::time_point parseDateYmd(const std::string& ymd) {
        std::tm tm{};
        std::istringstream ss(ymd);
        ss >> std::get_time(&tm, "%Y-%m-%d");
        if (ss.fail()) {
            throw std::runtime_error("Invalid date: " + ymd);
        }
        return std::chrono::system_clock::from_time_t(std::mktime(&tm));
    }

    FxTrade* createTradeFromLine(const std::string& line) {
        auto items = splitByString(line, seperator);
        if (items.size() != 9) {
            throw std::runtime_error("Invalid number of fields in trade data");
        }

        FxTrade* trade;

        if (items[0] == FxTrade::FxSpotTradeType) {
            trade = new FxTrade(items[8], FxTrade::FxSpotTradeType);
        } else if (items[0] == FxTrade::FxForwardTradeType) {
            trade = new FxTrade(items[8], FxTrade::FxForwardTradeType);
        } else {
#ifndef NDEBUG
                std::cerr << "WARNING: Unknown FX type '" << items[0]
                        << "' for trade '" << items[8]
                        << "'. Defaulting to FxForwardTradeType.\n";
#endif
                trade = new FxTrade(items[8], FxTrade::FxForwardTradeType);
            }

        trade->setTradeDate(parseDateYmd(items[1]));
        trade->setInstrument(items[2] + items[3]);     // Ccy1 + Ccy2
        trade->setCounterparty(items[7]);
        trade->setNotional(std::stod(items[4]));
        trade->setRate(std::stod(items[5]));
        trade->setValueDate(parseDateYmd(items[6]));

        return trade;
    }
}

void FxTradeLoader::streamTrades(const TradeCallback& onTrade) {
    if (!onTrade) {
        throw std::invalid_argument("onTrade callback cannot be null");
    }

    std::ifstream stream(dataFile_);
    if (!stream.is_open()) {
        throw std::runtime_error("Cannot open file: " + dataFile_);
    }

    int lineCount = 0;
    std::string line;

    while (std::getline(stream, line)) {
        // First 2 lines are headers in the FxTrades.dat
        if (lineCount < 2) {
            ++lineCount;
            continue;
        }

        // Stop cleanly at END marker
        if (line.rfind("END", 0) == 0) {
            break;
        }

        try {
            onTrade(createTradeFromLine(line)); // caller owns trade
        } catch (const std::exception& e) {
            std::cerr << e.what() << std::endl;
        }

        ++lineCount;
    }
}

std::vector<ITrade*> FxTradeLoader::loadTrades() {
    std::vector<ITrade*> result;
    streamTrades([&](ITrade* trade) { result.push_back(trade); });
    return result;
}

std::string FxTradeLoader::getDataFile() const {
    return dataFile_;
}

void FxTradeLoader::setDataFile(const std::string& file) {
    if (file.empty()) {
        throw std::invalid_argument("Filename cannot be null");
    }

    dataFile_ = file;
}