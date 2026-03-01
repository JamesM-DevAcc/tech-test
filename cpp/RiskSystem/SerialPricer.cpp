#include "SerialPricer.h"
#include <stdexcept>

#include "../Pricers/GovBondPricingEngine.h"
#include "../Pricers/CorpBondPricingEngine.h"
#include "../Pricers/FxPricingEngine.h"

namespace {
    std::string getEngineKey(const std::string& typeName)
    {
        const std::size_t pos = typeName.find_last_of('.');
        if (pos == std::string::npos) { return typeName; }
        return typeName.substr(pos + 1);
    }
}

SerialPricer::~SerialPricer() {
    for (auto& kv : pricers_) {
        delete kv.second;
    }
    pricers_.clear();
}

void SerialPricer::loadPricers()
{
    if (!pricers_.empty()) {
        return;
    }

    PricingConfigLoader loader;
    loader.setConfigFile("./PricingConfig/PricingEngines.xml");
    const PricingEngineConfig cfg = loader.loadConfig();

    for (const auto& item : cfg) {
        const std::string tradeType = item.getTradeType();
        const std::string typeName  = item.getTypeName();

        if (tradeType.empty())
            throw std::invalid_argument("Trade type not specified in config");

        if (typeName.empty())
            throw std::invalid_argument("Pricing engine type not specified for trade type: " + tradeType);

        const std::string engineId = getEngineKey(typeName);

        IPricingEngine* engine;
        if (engineId == "GovBondPricingEngine") {
            engine = new GovBondPricingEngine();
        } else if (engineId == "CorpBondPricingEngine") {
            engine = new CorpBondPricingEngine();
        } else if (engineId == "FxPricingEngine") {
            engine = new FxPricingEngine();
        } else {
            throw std::runtime_error("Unknown pricing engine type: " + typeName);
        }

        auto [it, inserted] = pricers_.emplace(tradeType, engine);
        if (!inserted) {
            delete engine;
            throw std::runtime_error("Duplicate pricer mapping for trade type: " + tradeType);
        }
    }
}

void SerialPricer::price(const std::vector<std::vector<ITrade*>>& tradeContainers, 
                         IScalarResultReceiver* resultReceiver) {
    loadPricers();
    
    for (const auto& tradeContainer : tradeContainers) {
        for (ITrade* trade : tradeContainer) {
            std::string tradeType = trade->getTradeType();
            if (pricers_.find(tradeType) == pricers_.end()) {
                resultReceiver->addError(trade->getTradeId(), "No Pricing Engines available for this trade type");
                continue;
            }
            
            IPricingEngine* pricer = pricers_[tradeType];
            pricer->price(trade, resultReceiver);
        }
    }
}