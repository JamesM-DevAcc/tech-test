#include "../RiskSystem/SerialTradeLoader.h"
#include "../RiskSystem/StreamingTradeLoader.h"
#include "../Models/ScalarResults.h"
#include "../RiskSystem/SerialPricer.h"
#include "../RiskSystem/ParallelPricer.h"
#include "../RiskSystem/ScreenResultPrinter.h"
#include <iostream>
#include <string>

#ifdef _WIN32
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

int _getch() {
    struct termios oldt, newt;
    int ch;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}
#endif

int main(int argc, char* argv[]) {

    int parallel = 0;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "parallel=enabled")  parallel = 1;
        if (arg == "parallel=disabled") parallel = 0;
    }

    std::cout << (parallel ? "Parallel Pricer in use" : "Serial Pricer in use") << "\n";

    SerialTradeLoader tradeLoader;
    auto allTrades = tradeLoader.loadTrades();
    
    ScalarResults results;

    switch(parallel) {
        case 1: {
            ParallelPricer pricer;
            pricer.price(allTrades, &results);
            break;
        }
        case 0:
        default: {
            SerialPricer pricer;
            pricer.price(allTrades, &results);
            break;
        }
    }
    
    ScreenResultPrinter screenPrinter;
    screenPrinter.printResults(results);

    std::cout << "Press any key to exit.." << std::endl;
    _getch();
    
    return 0;
}
