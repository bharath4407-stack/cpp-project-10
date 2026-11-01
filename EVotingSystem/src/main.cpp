// main.cpp - Entry point. Handles command-line options and top-level error handling.
//
// Options:  --data <folder>   use a different data folder (default: ./data)
//           --no-color        disable ANSI colours
//           --ascii           use '#' instead of the Unicode block in bar charts
//           --help
#include "ConsoleUI.h"
#include "Exceptions.h"
#include "Utils.h"
#include "VotingSystem.h"

#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    std::string dataDir = "data";
    bool noColor = false, ascii = false;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--data" && i + 1 < argc) dataDir = argv[++i];
        else if (arg == "--no-color") noColor = true;
        else if (arg == "--ascii") ascii = true;
        else {
            std::cout << "E-Voting System (Local Simulation)\n"
                         "Usage: evoting [--data <folder>] [--no-color] [--ascii]\n";
            return arg == "--help" ? 0 : 1;
        }
    }

    Utils::initConsole(noColor, ascii);
    try {
        VotingSystem system(dataDir);
        system.initialize();
        ConsoleUI ui(system);
        ui.run();
    } catch (const InputClosedException&) {
        std::cout << "\n\n  Input closed. Exiting safely - all data was already saved.\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\nFatal error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
