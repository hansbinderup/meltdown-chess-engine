#pragma once

#include "src/bit_board.h"
#include "src/evaluation/evaluator.h"
#include "src/file_logger.h"
#include "src/input_parsing.h"

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
        } else if (command == "debug") {
            return handleDebug(args);
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

    static bool handlePosition(std::string_view input)
    {
        auto [command, args] = split_sv_by_space(input);
        if (command == "startpos") {
            s_bitBoard.reset();
            const auto subCommand = sv_next_split(args);

            if (subCommand == "moves") {
                while (const auto move = parsing::moveFromString(s_bitBoard, args.substr(0, 4))) {
                    s_bitBoard.performMove(move.value());

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

        static evaluation::Evaluator evaluator(s_fileLogger);
        const auto bestMove = evaluator.getBestMove(s_bitBoard);

        std::cout << "bestmove " << bestMove.toString() << "\n";
        return true;
    }

    static bool handleDebug(std::string_view input)
    {
        auto [command, args] = split_sv_by_space(input);
        if (command == "position") {
            s_bitBoard.printBoardDebug();
            static evaluation::Evaluator evaluator(s_fileLogger);
            evaluator.printEvaluation(s_bitBoard);
        }

        return true;
    }

    static inline bool s_isRunning = false;
    static inline FileLogger s_fileLogger { "/tmp/uci.log" };
    static inline BitBoard s_bitBoard { s_fileLogger };
};
