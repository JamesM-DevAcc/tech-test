#ifndef FXTRADELOADER_H
#define FXTRADELOADER_H

#include "ITradeLoader.h"
#include "../Models/FxTrade.h"
#include <string>
#include <vector>

class FxTradeLoader : public ITradeLoader {
private:
    std::string dataFile_;
    
public:
    std::vector<ITrade*> loadTrades() override;
    void streamTrades(const TradeCallback& onTrade) override;

    std::string getDataFile() const override;
    void setDataFile(const std::string& file) override;
};

#endif // FXTRADELOADER_H
