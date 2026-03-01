#ifndef PARALLELPRICER_H
#define PARALLELPRICER_H

#include "../Models/IPricingEngine.h"
#include "../Models/ITrade.h"
#include "../Models/IScalarResultReceiver.h"
#include "PricingConfigLoader.h"

#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

class ParallelPricer {
private:
    std::map<std::string, std::string> pricerTypeNamesByTradeType_;
    std::mutex resultMutex_;

    void loadPricers();

public:
    ~ParallelPricer();

    void price(const std::vector<std::vector<ITrade*>>& tradeContainers,
               IScalarResultReceiver* resultReceiver);
};

#endif // PARALLELPRICER_H