#include "ScreenResultPrinter.h"
#include <iostream>

void ScreenResultPrinter::printResults(ScalarResults& results) {
    for (const auto& result : results) {
        std::cout << result.getTradeId();

        if (result.getResult().has_value()) {
            std::cout << " : " << *result.getResult();
        }

        if (result.getError().has_value()) {
            std::cout << " : " << *result.getError();
        }

        std::cout << '\n';
    }
}
