#pragma once

#include "evaluation/evaluator.h"
#include "evaluation/perft.h"
#include "parsing/fen_parser.h"
#include "parsing/input_parsing.h"

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

        s_board.reset();
        engine::TtHashTable::clear();
        s_evaluator.reset();

        std::array<char, s_inputBufferSize> buffer;
        while (s_isRunning && std::cin.getline(buffer.data(), buffer.size())) {
            processInput(std::string_view(buffer.data()));
        }
    }

private:
    static bool processInput(std::string_view input)
    {
        const auto [command, args] = parsing::split_sv_by_space(input);

        if (command == "uci") {
            return handleUci();
        } else if (command == "isready") {
            return handleIsReady();
        } else if (command == "position") {
            return handlePosition(args);
        } else if (command == "ucinewgame") {
            return handleUcinewgame();
        } else if (command == "go") {
            return handleGo(args);
        } else if (command == "debug") {
            return handleDebug(args);
        } else if (command == "perft") {
            return handlePerft(args);
        } else if (command == "help") {
            return handleHelp();
        } else if (command == "quit") {
            s_isRunning = false;
        } else {
            // invalid input
            return false;
        }

        /* ensure responses are printed immediately */
        fflush(stdout);

        return true;
    }

    static bool handleUci()
    {
        fmt::println("id engine Meltdown\n"
                     "id author Hans Binderup\n"
                     "uciok");

        return true;
    }

    static bool handleIsReady()
    {
        fmt::println("readyok");

        return true;
    }

    static bool handlePosition(std::string_view input)
    {
        auto [command, args] = parsing::split_sv_by_space(input);

        /* helper to iterate list of moves - can be passed as both fen string or startpos */
        constexpr const auto iterateMovesFnc = [](std::string_view sv) {
            uint64_t hash = engine::generateHashKey(s_board);
            while (true) {
                const auto moveSv = parsing::sv_next_split(sv);
                const auto move = parsing::moveFromString(s_board, moveSv.value_or(sv));

                if (move.has_value()) {
                    s_board = engine::performMove(s_board, move.value(), hash);
                    s_evaluator.updateRepetition(hash);
                } else {
                    break;
                }
            }
        };

        if (command == "startpos") {
            s_board.reset();

            s_evaluator.reset();
            engine::TtHashTable::advanceGeneration();

            const auto subCommand = parsing::sv_next_split(args);
            if (subCommand == "moves") {
                iterateMovesFnc(args);
            }
        } else if (command == "fen") {
            const auto board = parsing::FenParser::parse(args);
            if (board.has_value()) {
                s_board = std::move(board.value());

                s_evaluator.reset();
                engine::TtHashTable::advanceGeneration();

                using namespace std::string_view_literals;
                const auto movesPos = args.rfind("moves ");
                if (movesPos != std::string_view::npos) {
                    args = args.substr(movesPos + "moves "sv.size());
                    iterateMovesFnc(args);
                }
            }
        }

        return true;
    }

    static bool handleUcinewgame()
    {
        s_board.reset();
        s_evaluator.reset();
        engine::TtHashTable::clear();

        return true;
    }

    static bool handleGo(std::string_view args)
    {
        std::optional<uint8_t> depth;

        s_evaluator.resetTiming();

        while (const auto setting = parsing::sv_next_split(args)) {
            const auto value = parsing::sv_next_split(args);
            const auto valNum = parsing::to_number(value.value_or(args));

            if (setting == "wtime") {
                s_evaluator.setWhiteTime(valNum.value_or(0));
            } else if (setting == "btime") {
                s_evaluator.setBlackTime(valNum.value_or(0));
            } else if (setting == "movestogo") {
                s_evaluator.setMovesToGo(valNum.value_or(0));
            } else if (setting == "movetime") {
                s_evaluator.setMoveTime(valNum.value_or(0));
            } else if (setting == "winc") {
                s_evaluator.setWhiteMoveInc(valNum.value_or(0));
            } else if (setting == "binc") {
                s_evaluator.setBlackMoveInc(valNum.value_or(0));
            } else if (setting == "depth") {
                depth = valNum;
            }
        }

        const auto bestMove = s_evaluator.getBestMove(s_board, depth);

        fmt::println("bestmove {}", bestMove);

        return true;
    }

    static bool handleDebug(std::string_view input)
    {
        auto [command, args] = parsing::split_sv_by_space(input);
        if (command == "position") {
            engine::printPositionDebug(s_board);
        } else if (command == "eval") {
            const auto depth = parsing::to_number(args);
            s_evaluator.printEvaluation(s_board, depth);
        } else if (command == "clear") {
            s_evaluator.reset();
            engine::TtHashTable::clear();
        }

        return true;
    }

    static bool handlePerft(std::string_view args)
    {
        const auto depth = parsing::to_number(args);
        if (depth.has_value()) {
            Perft::run(s_board, depth.value());
        } else {
            fmt::println("invalid input: {}", args);
        }

        return true;
    }

    static bool handleHelp()
    {
        fmt::print("\nMeltdown communicates over UCI protocol.\n"
                   "Most common handles are implemented.\n\n"
                   "Additional Meltdown options:\n"
                   "============================================================================\n"
                   "debug eval <depth>  :  print evaluation - seen from player to move\n"
                   "debug position      :  print the current position\n"
                   "debug clear         :  clear all scoring tables\n"
                   "version             :  print version information\n"
                   "quit                :  stop the engine\n");

        return true;
    }

    static inline bool s_isRunning = false;
    static inline BitBoard s_board {};
    static inline evaluation::Evaluator s_evaluator;

    constexpr static inline std::size_t s_inputBufferSize { 2048 };
};
