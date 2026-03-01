#include "ScalarResults.h"

#include <stdexcept>

ScalarResults::~ScalarResults() = default;

std::optional<ScalarResult> ScalarResults::operator[](const std::string& tradeId) const {
    if (!containsTrade(tradeId)) {
        return std::nullopt;
    }

    std::optional<double> priceResult = std::nullopt;
    std::optional<std::string> error = std::nullopt;

    auto resultIt = results_.find(tradeId);
    if (resultIt != results_.end()) {
        priceResult = resultIt->second;
    }

    auto errorIt = errors_.find(tradeId);
    if (errorIt != errors_.end()) {
        error = errorIt->second;
    }

    return ScalarResult(tradeId, priceResult, error);
}

bool ScalarResults::containsTrade(const std::string& tradeId) const {
    return results_.find(tradeId) != results_.end() || errors_.find(tradeId) != errors_.end();
}

void ScalarResults::addResult(const std::string& tradeId, double result) {
    results_[tradeId] = result;
}

void ScalarResults::addError(const std::string& tradeId, const std::string& error) {
    errors_[tradeId] = error;
}

ScalarResults::Iterator::Iterator(
    const ScalarResults* parent,
    std::map<std::string, double>::const_iterator resultsIt,
    std::map<std::string, std::string>::const_iterator errorsIt)
    : parent_(parent), resultsIt_(resultsIt), errorsIt_(errorsIt) {
    updateCurrentTradeId();
}

void ScalarResults::Iterator::updateCurrentTradeId() {
    if (!parent_) {
        currentTradeId_.clear();
        return;
    }

    const bool resultsEnd = (resultsIt_ == parent_->results_.cend());
    const bool errorsEnd = (errorsIt_ == parent_->errors_.cend());

    if (resultsEnd && errorsEnd) {
        currentTradeId_.clear();
        return;
    }

    if (resultsEnd) {
        currentTradeId_ = errorsIt_->first;
        return;
    }

    if (errorsEnd) {
        currentTradeId_ = resultsIt_->first;
        return;
    }

    currentTradeId_ = (resultsIt_->first < errorsIt_->first) ? resultsIt_->first : errorsIt_->first;
}

ScalarResults::Iterator& ScalarResults::Iterator::operator++() {
    if (!parent_) {
        return *this;
    }

    if (resultsIt_ == parent_->results_.cend() && errorsIt_ == parent_->errors_.cend()) {
        return *this; // already at end
    }

    const std::string tradeId = currentTradeId_;

    // Advance any iterator(s) that are currently pointing at the trade we just yielded
    if (resultsIt_ != parent_->results_.cend() && resultsIt_->first == tradeId) {
        ++resultsIt_;
    }
    if (errorsIt_ != parent_->errors_.cend() && errorsIt_->first == tradeId) {
        ++errorsIt_;
    }

    updateCurrentTradeId();
    return *this;
}

ScalarResult ScalarResults::Iterator::operator*() const {
    if (!parent_ || (resultsIt_ == parent_->results_.cend() && errorsIt_ == parent_->errors_.cend())) {
        throw std::out_of_range("Attempt to dereference end iterator");
    }

    std::optional<double> result = std::nullopt;
    std::optional<std::string> error = std::nullopt;

    if (resultsIt_ != parent_->results_.cend() && resultsIt_->first == currentTradeId_) {
        result = resultsIt_->second;
    }
    if (errorsIt_ != parent_->errors_.cend() && errorsIt_->first == currentTradeId_) {
        error = errorsIt_->second;
    }

    return ScalarResult(currentTradeId_, result, error);
}

bool ScalarResults::Iterator::operator!=(const Iterator& other) const {
    return parent_ != other.parent_ || resultsIt_ != other.resultsIt_ || errorsIt_ != other.errorsIt_;
}

ScalarResults::Iterator ScalarResults::begin() const {
    return Iterator(this, results_.cbegin(), errors_.cbegin());
}

ScalarResults::Iterator ScalarResults::end() const {
    return Iterator(this, results_.cend(), errors_.cend());
}