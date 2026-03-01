#include "ParallelPricer.h"

#include "../Pricers/CorpBondPricingEngine.h"
#include "../Pricers/FxPricingEngine.h"
#include "../Pricers/GovBondPricingEngine.h"

#include <atomic>
#include <stdexcept>
#include <thread>
#include <vector>

namespace {
    class ThreadSafeScalarResultReceiver : public IScalarResultReceiver {
    public:
        ThreadSafeScalarResultReceiver(IScalarResultReceiver* inner, std::mutex& m)
            : inner_(inner), mutex_(m) {
            if (inner_ == nullptr) {
                throw std::invalid_argument("resultReceiver cannot be null");
            }
        }

        void addResult(const std::string& tradeId, double result) override {
            std::lock_guard<std::mutex> lock(mutex_);
            inner_->addResult(tradeId, result);
        }

        void addError(const std::string& tradeId, const std::string& error) override {
            std::lock_guard<std::mutex> lock(mutex_);
            inner_->addError(tradeId, error);
        }

    private:
        IScalarResultReceiver* inner_;
        std::mutex& mutex_;
    };

    IPricingEngine* createEngineFromTypeName(const std::string& typeName) {
        if (typeName.find("GovBondPricingEngine") != std::string::npos) {
            return new GovBondPricingEngine();
        }
        if (typeName.find("CorpBondPricingEngine") != std::string::npos) {
            return new CorpBondPricingEngine();
        }
        if (typeName.find("FxPricingEngine") != std::string::npos) {
            return new FxPricingEngine();
        }

        throw std::runtime_error("Unknown pricing engine type: " + typeName);
    }
}

ParallelPricer::~ParallelPricer() = default;

void ParallelPricer::loadPricers() {
    if (!pricerTypeNamesByTradeType_.empty()) {
        return; 
    }

    PricingConfigLoader pricingConfigLoader;
    pricingConfigLoader.setConfigFile("./PricingConfig/PricingEngines.xml");
    PricingEngineConfig pricerConfig = pricingConfigLoader.loadConfig();

    for (const auto& configItem : pricerConfig) {
        const std::string tradeType = configItem.getTradeType();
        const std::string typeName  = configItem.getTypeName();

        if (tradeType.empty()) {
            throw std::invalid_argument("Trade type not specified in config");
        }
        if (typeName.empty()) {
            throw std::invalid_argument("Pricing engine type not specified for trade type: " + tradeType);
        }

        (void)createEngineFromTypeName(typeName);

        auto [it, inserted] = pricerTypeNamesByTradeType_.emplace(tradeType, typeName);
        if (!inserted) {
            throw std::runtime_error("Duplicate pricer mapping for trade type: " + tradeType);
        }
    }
}

void ParallelPricer::price(const std::vector<std::vector<ITrade*>>& tradeContainers,
                           IScalarResultReceiver* resultReceiver) {
    if (resultReceiver == nullptr) {
        throw std::invalid_argument("resultReceiver cannot be null");
    }

    loadPricers();

    std::vector<ITrade*> trades;
    for (const auto& container : tradeContainers) {
        for (ITrade* trade : container) {
            if (trade != nullptr) {
                trades.push_back(trade);
            }
        }
    }

    if (trades.empty()) {
        return;
    }

    ThreadSafeScalarResultReceiver safeReceiver(resultReceiver, resultMutex_);

    unsigned int threadCount = std::thread::hardware_concurrency();
    if (threadCount == 0) {
        threadCount = 4; 
    }
    if (threadCount > trades.size()) {
        threadCount = static_cast<unsigned int>(trades.size());
    }
    if (threadCount == 0) {
        threadCount = 1;
    }

    std::atomic<size_t> nextIndex{0};

    auto worker = [&]() {
        while (true) {
            const size_t i = nextIndex.fetch_add(1, std::memory_order_relaxed); 
            if (i >= trades.size()) {
                return;
            }

            ITrade* trade = trades[i];
            const std::string tradeType = trade->getTradeType();

            auto it = pricerTypeNamesByTradeType_.find(tradeType);
            if (it == pricerTypeNamesByTradeType_.end()) {
                safeReceiver.addError(trade->getTradeId(), "No Pricing Engines available for this trade type");
                continue;
            }

            try {
                auto pricer = createEngineFromTypeName(it->second);
                pricer->price(trade, &safeReceiver);
            } catch (const std::exception& ex) {
                safeReceiver.addError(trade->getTradeId(), ex.what());
            } catch (...) {
                safeReceiver.addError(trade->getTradeId(), "Unknown pricing error");
            }
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(threadCount);
    for (unsigned int t = 0; t < threadCount; ++t) {
        threads.emplace_back(worker);
    }

    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }
}