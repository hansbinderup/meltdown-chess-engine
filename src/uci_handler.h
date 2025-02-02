#pragma once

#include "bit_board.h"
#include "file_logger.h"

#include <iostream>
#include <string_view>

class UciHandler {
public:
    // no need to create an object - the methods are purely to group the
    // functionality
    UciHandler() = delete;

    static void run()
    {
        s_fileLogger.log("\n\n*** Running UciHandler! ***\n");
        s_isRunning = true;

        std::string input;
        while (s_isRunning && std::getline(std::cin, input)) {
            processInput(input);
        }
    }

private:
    static std::pair<std::string_view, std::string_view> split_sv_by_space(std::string_view sv)
    {
        const auto firstSep = sv.find_first_of(' ');
        const auto first = sv.substr(0, firstSep);
        const auto second = firstSep == std::string_view::npos
            ? ""
            : sv.substr(firstSep + 1);

        return std::make_pair(first, second);
    }

    static std::optional<std::string_view> sv_next_split(std::string_view& sv)
    {
        const auto firstSep = sv.find_first_of(' ');

        if (firstSep == std::string_view::npos) {
            return std::nullopt;
        }

        const auto ret = sv.substr(0, firstSep);
        sv = sv.substr(firstSep + 1);

        return ret;
    }

    static bool processInput(std::string_view input)
    {
        const auto [command, args] = split_sv_by_space(input);
        /* s_fileLogger.log("processInput, command: {}, args: {}", command, args); */

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
            s_fileLogger.log("processing input failed!");
            return false;
        }

        return true;
    }

    static bool handleUci()
    {
        std::cout << "id engine Meltdown\n"
                  << "id author Hans Binderup\n"
                  << "uciok\n";

        return true;
    }

    static bool handleIsReady()
    {
        std::cout << "readyok\n";

        return true;
    }

    static std::optional<movement::Move> move_from_input(std::string_view sv)
    {
        if (sv.size() < 4) {
            return std::nullopt;
        }

        // quick and dirty way to transform move string to integer values
        const uint8_t fromIndex = (sv.at(0) - 'a') + (sv.at(1) - '1') * 8;
        const uint8_t toIndex = (sv.at(2) - 'a') + (sv.at(3) - '1') * 8;

        return movement::Move { fromIndex, toIndex };
    }

    static std::string move_to_string(movement::Move move)
    {
        std::string result;
        result.resize(4); // Preallocate space

        result[0] = 'a' + (move.from % 8); // Column
        result[1] = '1' + (move.from / 8); // Row
        result[2] = 'a' + (move.to % 8); // Column
        result[3] = '1' + (move.to / 8); // Row

        return result;
    }

    static bool handlePosition(std::string_view input)
    {
        auto [command, args] = split_sv_by_space(input);
        if (command == "startpos") {
            s_bitBoard.reset();
            const auto subCommand = sv_next_split(args);

            if (subCommand == "moves") {
                while (const auto move = move_from_input(args.substr(0, 4))) {
                    s_bitBoard.perform_move(move.value());

                    auto spaceSep = args.find(' ');
                    if (spaceSep == std::string_view::npos) {
                        break;
                    }

                    args = args.substr(spaceSep + 1);
                }
            }
        }

        return true;
    }

    static bool handleGo(std::string_view args)
    {
        std::ignore = args;

        const auto bestMove = s_bitBoard.getBestMove();

        std::cout << "bestmove " << move_to_string(bestMove) << "\n";
        return true;
    }

    static inline bool s_isRunning = false;
    static inline FileLogger s_fileLogger { "/tmp/uci.log" };
    static inline BitBoard s_bitBoard { s_fileLogger };
};
