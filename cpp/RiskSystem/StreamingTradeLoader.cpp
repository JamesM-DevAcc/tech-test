#include "StreamingTradeLoader.h"

#include "../Loaders/BondTradeLoader.h"
#include "../Loaders/FxTradeLoader.h"
#include "PricingConfigLoader.h"

#include "../Pricers/GovBondPricingEngine.h"
#include "../Pricers/CorpBondPricingEngine.h"
#include "../Pricers/FxPricingEngine.h"

#include <stdexcept>

std::vector<ITradeLoader*> StreamingTradeLoader::getTradeLoaders() {
    std::vector<ITradeLoader*> loaders;
    
    BondTradeLoader* bondLoader = new BondTradeLoader();
    bondLoader->setDataFile("TradeData/BondTrades.dat");
    loaders.push_back(bondLoader);
    
    FxTradeLoader* fxLoader = new FxTradeLoader();
    fxLoader->setDataFile("TradeData/FxTrades.dat");
    loaders.push_back(fxLoader);
    
    return loaders;
}

static IPricingEngine* createEngineFromTypeName(const std::string& typeName) {
    // XML typeName is usually a fully qualified name, so just "contains" checks is safest.
    if (typeName.find("GovBondPricingEngine") != std::string::npos) {
        return new GovBondPricingEngine();
    }
    if (typeName.find("CorpBondPricingEngine") != std::string::npos) {
        return new CorpBondPricingEngine();
    }
    if (typeName.find("FxPricingEngine") != std::string::npos) {
        return new FxPricingEngine;
    }

    throw std::runtime_error("Unknown pricing engine type: " + typeName);
}

void StreamingTradeLoader::loadPricers() {
    if (!pricers_.empty()) {
        return;
    }

    PricingConfigLoader loader;
    loader.setConfigFile("./PricingConfig/PricingEngines.xml");
    PricingEngineConfig cfg = loader.loadConfig();

    for (const auto& item : cfg) {
        const std::string tradeType = item.getTradeType();
        const std::string typeName = item.getTypeName();

        pricers_[tradeType] = createEngineFromTypeName(typeName);
    }
}

StreamingTradeLoader::~StreamingTradeLoader() {
    
}

void StreamingTradeLoader::loadAndPrice(IScalarResultReceiver* resultReceiver) {
    if (resultReceiver == nullptr) {
        throw std::invalid_argument("resultReceiver cannot be null");
    }

    loadPricers();
    auto loaders = getTradeLoaders();

    for (auto& loader : loaders) {
        loader->streamTrades([&](ITrade* trade) {
            if (trade == nullptr) {
                return;
            }

            auto it = pricers_.find(trade->getTradeType());
            if (it == pricers_.end()) {
                resultReceiver->addError(
                    trade->getTradeId(),
                    "No Pricing Engines available for this trade type"
                );
            } else {
                it->second->price(trade, resultReceiver);
            }

            // key point: do NOT keep the trade population in memory
            delete trade;
        });
    }
}
