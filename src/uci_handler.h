#pragma once

#include "src/evaluation/evaluator.h"
#include "src/evaluation/perft.h"
#include "src/file_logger.h"
#include "src/parsing/fen_parser.h"
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
        s_board.reset();

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

        /* helper to iterate list of moves - can be passed as both fen string or startpos */
        constexpr const auto iterateMovesFnc = [](std::string_view sv) {
            while (true) {
                const auto moveSv = parsing::sv_next_split(sv);
                const auto move = parsing::moveFromString(s_board, moveSv.value_or(sv));

                if (move.has_value()) {
                    uint64_t hash {}; // TODO: make it optional to include
                    s_board = engine::performMove(s_board, move.value(), hash);
                } else {
                    break;
                }
            }
        };

        if (command == "startpos") {
            s_board.reset();
            const auto subCommand = parsing::sv_next_split(args);
            if (subCommand == "moves") {
                iterateMovesFnc(args);
            }
        } else if (command == "fen") {
            const auto board = parsing::FenParser::parse(args);
            if (board.has_value()) {
                s_board = std::move(board.value());

                using namespace std::string_view_literals;
                const auto movesPos = args.rfind("moves ");
                if (movesPos != std::string_view::npos) {
                    args = args.substr(movesPos + "moves "sv.size());
                    iterateMovesFnc(args);
                }
            } else {
                s_fileLogger.log("Fen parsing failed: {}", args);
            }
        }

        return true;
    }

    static bool handleGo(std::string_view args)
    {
        static evaluation::Evaluator evaluator(s_fileLogger);
        std::optional<uint8_t> depth;

        evaluator.reset();
        while (const auto setting = parsing::sv_next_split(args)) {
            const auto value = parsing::sv_next_split(args);
            const auto valNum = parsing::to_number(value.value_or(args));

            if (setting == "wtime") {
                evaluator.setWhiteTime(valNum.value_or(0));
            } else if (setting == "btime") {
                evaluator.setBlackTime(valNum.value_or(0));
            } else if (setting == "movestogo") {
                evaluator.setMovesToGo(valNum.value_or(0));
            } else if (setting == "movetime") {
                evaluator.setMoveTime(valNum.value_or(0));
            } else if (setting == "winc") {
                evaluator.setWhiteMoveInc(valNum.value_or(0));
            } else if (setting == "binc") {
                evaluator.setBlackMoveInc(valNum.value_or(0));
            } else if (setting == "depth") {
                depth = valNum;
            }
        }

        const auto bestMove = evaluator.getBestMove(s_board, depth);

        std::cout << "bestmove " << bestMove.toString().data() << "\n";
        return true;
    }

    static bool handleDebug(std::string_view input)
    {
        auto [command, args] = parsing::split_sv_by_space(input);
        if (command == "position") {
            engine::printBoardDebug(s_fileLogger, s_board);
            static evaluation::Evaluator evaluator(s_fileLogger);
            evaluator.printEvaluation(s_board);
        }

        return true;
    }

    static bool handlePerft(std::string_view args)
    {
        const auto depth = parsing::to_number(args);
        if (depth.has_value()) {
            Perft::run(s_board, depth.value());
        } else {
            std::cout << "invalid input: " << args << std::endl;
        }

        return true;
    }

    static inline bool s_isRunning = false;
    static inline FileLogger s_fileLogger { "/tmp/uci.log" };
    static inline BitBoard s_board {};

    constexpr static inline std::size_t s_inputBufferSize { 2048 };
};
