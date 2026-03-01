#ifndef ITRADELOADER_H
#define ITRADELOADER_H

#include "../Models/ITrade.h"
#include <vector>
#include <string>
#include <functional>

class ITradeLoader {
public:
    using TradeCallback = std::function<void(ITrade*)>;
    
    virtual ~ITradeLoader() = default;
    virtual std::vector<ITrade*> loadTrades() = 0;
    virtual  void streamTrades(const TradeCallback& onTrade) {
        std::vector<ITrade*> trades = loadTrades();
        for (ITrade* t : trades) {
            onTrade(t);
        }
    }
    virtual std::string getDataFile() const = 0;
    virtual void setDataFile(const std::string& file) = 0;
};

#endif // ITRADELOADER_H
