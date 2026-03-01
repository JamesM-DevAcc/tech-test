#ifndef SCALARRESULTS_H
#define SCALARRESULTS_H

#include "IScalarResultReceiver.h"
#include "ScalarResult.h"

#include <iterator>
#include <map>
#include <optional>
#include <string>

class ScalarResults : public IScalarResultReceiver {
public:
    ~ScalarResults() override;

    std::optional<ScalarResult> operator[](const std::string& tradeId) const;
    bool containsTrade(const std::string& tradeId) const;

    void addResult(const std::string& tradeId, double result) override;
    void addError(const std::string& tradeId, const std::string& error) override;

    class Iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = ScalarResult;
        using difference_type = std::ptrdiff_t;
        using pointer = ScalarResult*;
        using reference = ScalarResult&;

        Iterator() = default;

        // Constructable from ScalarResults parent (needed for begin/end)
        Iterator(const ScalarResults* parent,
                 std::map<std::string, double>::const_iterator resultsIt,
                 std::map<std::string, std::string>::const_iterator errorsIt);

        Iterator& operator++();
        ScalarResult operator*() const;
        bool operator!=(const Iterator& other) const;

    private:
        void updateCurrentTradeId();

        const ScalarResults* parent_ = nullptr;
        std::map<std::string, double>::const_iterator resultsIt_{};
        std::map<std::string, std::string>::const_iterator errorsIt_{};
        std::string currentTradeId_;
    };

    Iterator begin() const;
    Iterator end() const;

private:
    std::map<std::string, double> results_;
    std::map<std::string, std::string> errors_;
};

#endif // SCALARRESULTS_H