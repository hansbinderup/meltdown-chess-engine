#pragma once

#include "src/engine.h"
#include "src/evaluation/evaluator.h"
#include "src/evaluation/perft.h"
#include "src/file_logger.h"
#include "src/parsing/input_parsing.h"

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

        std::array<char, s_inputBufferSize> buffer;
        while (s_isRunning && std::cin.getline(buffer.data(), buffer.size())) {
            processInput(std::string_view(buffer.data()));
        }
    }

private:
    static bool processInput(std::string_view input)
    {
        const auto [command, args] = parsing::split_sv_by_space(input);
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
        } else if (command == "perft") {
            return handlePerft(args);
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
        auto [command, args] = parsing::split_sv_by_space(input);
        if (command == "startpos") {
            s_engine.reset();
            const auto subCommand = parsing::sv_next_split(args);

            if (subCommand == "moves") {
                while (true) {
                    auto spaceSep = args.find(' ');
                    if (spaceSep == std::string_view::npos) {
                        // ensure that we also parse move even if it's the last one
                        const auto move = parsing::moveFromString(s_engine, args);
                        if (move.has_value()) {
                            s_engine.performMove(move.value());
                        }

                        break;
                    }

                    const auto move = parsing::moveFromString(s_engine, args.substr(0, spaceSep));

                    if (move.has_value()) {
                        s_engine.performMove(move.value());
                    } else {
                        std::cerr << "No move found! args: " << args << std::endl;
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
        const auto bestMove = evaluator.getBestMove(s_engine);

        std::cout << "bestmove " << bestMove.toString().data() << "\n";
        return true;
    }

    static bool handleDebug(std::string_view input)
    {
        auto [command, args] = parsing::split_sv_by_space(input);
        if (command == "position") {
            s_engine.printBoardDebug();
            static evaluation::Evaluator evaluator(s_fileLogger);
            evaluator.printEvaluation(s_engine);
        }

        return true;
    }

    static bool handlePerft(std::string_view args)
    {
        const auto depth = parsing::to_number(args);
        if (depth.has_value()) {
            Perft::run(s_engine, depth.value());
        } else {
            std::cout << "invalid input: " << args << std::endl;
        }

        return true;
    }

    static inline bool s_isRunning = false;
    static inline FileLogger s_fileLogger { "/tmp/uci.log" };
    static inline Engine s_engine { s_fileLogger };

    constexpr static inline std::size_t s_inputBufferSize { 2048 };
};
