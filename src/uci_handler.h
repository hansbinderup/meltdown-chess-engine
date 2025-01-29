#pragma once

#include <iostream>
#include <string_view>

class UciHandler {
   public:
    static void run() {
        is_running = true;

        std::string input;
        while (is_running && std::getline(std::cin, input)) {
            process_input(input);
        }
    }

   private:
    static bool process_input(std::string_view input) {
        std::cout << "input: " << input << std::endl;

        const auto command_sep = input.find_first_of(' ');
        const auto command = input.substr(0, command_sep);
        const auto args = command_sep == std::string_view::npos
                              ? ""
                              : input.substr(command_sep + 1);

        if (command == "uci") {
            return handle_uci();
        } else if (command == "isready") {
            return handle_is_ready();
        } else if (command == "position") {
            return handle_position(args);
        } else if (command == "go") {
            return handle_go(args);
        } else if (command == "quit") {
            is_running = false;
        } else {
            // invalid input
            return false;
        }

        return true;
    }

    static bool handle_uci() {
        std::cout << "id engine Meltdown" << std::endl;
        std::cout << "id author Hans Binderup" << std::endl;
        std::cout << "uciok" << std::endl;

        return true;
    }

    static bool handle_is_ready() {
        std::cout << "readyok" << std::endl;

        return true;
    }

    static bool handle_position(std::string_view args) {
        std::ignore = args;
        return true;
    }

    static bool handle_go(std::string_view args) {
        std::ignore = args;

        std::cout << "bestmove e2e4" << std::endl;
        return true;
    }

    static inline bool is_running = false;
};
