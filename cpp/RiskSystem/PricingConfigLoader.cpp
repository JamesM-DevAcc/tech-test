#include "PricingConfigLoader.h"
#include <stdexcept>
#include <fstream>
#include <sstream>

namespace {
    std::string attr(const std::string& tag, const char* key) {
        std::string attrPattern = std::string(key) + "=\"";
        size_t valueStart = tag.find(attrPattern);
        char quoteChar = '"';

        if (valueStart == std::string::npos) {
            attrPattern = std::string(key) + "='";
            valueStart = tag.find(attrPattern);
            quoteChar = '\'';
            if (valueStart == std::string::npos) {
                return {};
            }
        }
        valueStart += attrPattern.size();

        size_t valueEnd = tag.find(quoteChar, valueStart);
        if (valueEnd == std::string::npos) {
            return {};
        }

        return tag.substr(valueStart, valueEnd - valueStart);
    }
}

std::string PricingConfigLoader::getConfigFile() const {
    return configFile_;
}

void PricingConfigLoader::setConfigFile(const std::string& file) {
    configFile_ = file;
}

PricingEngineConfig PricingConfigLoader::loadConfig() {
    if (configFile_.empty()) {
        throw std::runtime_error("No config file set");
    }

    std::ifstream configStream(configFile_, std::ios::binary);
    if (!configStream) {
        throw std::runtime_error("Failed to open config file: " + configFile_);
    }

    std::ostringstream filecontents;
    filecontents << configStream.rdbuf();
    const std::string xmlText = filecontents.str();

    PricingEngineConfig configItems;
    for (size_t tagStart = 0; (tagStart = xmlText.find("<Engine ", tagStart)) != std::string::npos; ) {
        size_t tagEnd = xmlText.find('>', tagStart);
        if (tagEnd == std::string::npos) {
            throw std::runtime_error("Invalid XML: unterminated tag");
        }

        std::string tag = xmlText.substr(tagStart, tagEnd - tagStart + 1);
        std::string tradeType = attr(tag, "tradeType");
        std::string assembly  = attr(tag, "assembly");
        std::string typeName  = attr(tag, "pricingEngine");

        if (tradeType.empty() || assembly.empty() || typeName.empty()) {
            throw std::runtime_error("Missing attributes");
        }

        PricingEngineConfigItem item;
        item.setTradeType(tradeType);
        item.setAssembly(assembly);
        item.setTypeName(typeName);
        configItems.push_back(item);

        tagStart = tagEnd + 1;
    }

    return configItems;
}