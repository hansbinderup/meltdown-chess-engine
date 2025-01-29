#pragma once

#include <iostream>
#include <string_view>

class UciHandler {
public:
    // no need to create an object - the methods are purely to group the
    // functionality
    UciHandler() = delete;

    static void run()
    {
        s_isRunning = true;

        std::string input;
        while (s_isRunning && std::getline(std::cin, input)) {
            processInput(input);
        }
    }

private:
    static bool processInput(std::string_view input)
    {
        std::cout << "input: " << input << std::endl;

        const auto commandSep = input.find_first_of(' ');
        const auto command = input.substr(0, commandSep);
        const auto args = commandSep == std::string_view::npos
            ? ""
            : input.substr(commandSep + 1);

        if (command == "uci") {
            return handleUci();
        } else if (command == "isready") {
            return handleIsReady();
        } else if (command == "position") {
            return handlePosition(args);
        } else if (command == "go") {
            return handleGo(args);
        } else if (command == "quit") {
            s_isRunning = false;
        } else {
            // invalid input
            return false;
        }

        return true;
    }

    static bool handleUci()
    {
        std::cout << "id engine Meltdown" << std::endl;
        std::cout << "id author Hans Binderup" << std::endl;
        std::cout << "uciok" << std::endl;

        return true;
    }

    static bool handleIsReady()
    {
        std::cout << "readyok" << std::endl;

        return true;
    }

    static bool handlePosition(std::string_view args)
    {
        std::ignore = args;
        return true;
    }

    static bool handleGo(std::string_view args)
    {
        std::ignore = args;

        std::cout << "bestmove e2e4" << std::endl;
        return true;
    }

    static inline bool s_isRunning = false;
};
