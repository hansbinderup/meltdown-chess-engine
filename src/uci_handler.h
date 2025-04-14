#pragma once

#include "engine/bench.h"
#include "evaluation/evaluator.h"
#include "evaluation/perft.h"
#include "helpers/formatters.h"
#include "parsing/fen_parser.h"
#include "parsing/input_parsing.h"

#include "syzygy/syzygy.h"
#include "uci_options.h"
#include "version/version.h"

#include <iostream>
#include <string_view>
#include <thread>

class UciHandler {
public:
    // no need to create an object - the methods are purely to group the
    // functionality
    UciHandler() = delete;

    static void run()
    {
        s_board.reset();
        engine::TtHashTable::clear();
        s_evaluator.reset();

        startInputThread();

        /* always release resources */
        syzygy::deinit();
    }

private:
    static void startInputThread()
    {
        s_isRunning = true;

        std::thread inputThread([] {
            std::array<char, s_inputBufferSize> buffer;
            while (s_isRunning && std::cin.getline(buffer.data(), buffer.size())) {
                processInput(std::string_view(buffer.data()));
            }
        });

        inputThread.join();
    }

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
        } else if (command == "ponderhit") {
            return handlePonderhit();
        } else if (command == "stop") {
            return handleStop();
        } else if (command == "setoption") {
            return handleSetOption(args);
        } else if (command == "debug") {
            return handleDebug(args);
        } else if (command == "perft") {
            return handlePerft(args);
        } else if (command == "help") {
            return handleHelp();
        } else if (command == "authors") {
            return handleAuthors();
        } else if (command == "bench") {
            return handleBench(args);
        } else if (command == "version") {
            return handleVersion();
        } else if (command == "quit" || command == "exit") {
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
                     "id author Hans Binderup");

        for (const auto& option : s_uciOptions) {
            ucioption::printInfo(option);
        }

        fmt::println("uciok");

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

    static bool handlePonderhit()
    {
        s_evaluator.ponderhit(s_board);

        return true;
    }

    static bool handleGo(std::string_view args)
    {
        std::optional<uint8_t> depth;

        s_evaluator.resetTiming();

        while (const auto setting = parsing::sv_next_split(args)) {
            /* single word settings first */
            if (setting == "ponder") {
                depth = s_maxSearchDepth; /* continue searching until told to stop */
                continue;
            }

            /* settings with values */
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

        std::thread searchThread([depth] {
            const auto bestMove = s_evaluator.getBestMove(s_board, depth);
            fmt::print("bestmove {}", bestMove);
            if (s_ponderingEnabled) {
                const auto ponderMove = s_evaluator.getPonderMove();
                if (ponderMove.has_value()) {
                    fmt::print(" ponder {}", *ponderMove);
                }
            }

            fmt::print("\n");
            fflush(stdout);
        });

        searchThread.detach();

        return true;
    }

    static bool handleStop()
    {
        s_evaluator.stop();

        return true;
    }

    static bool handleSetOption(std::string_view input)
    {
        const auto name = parsing::sv_next_split(input);
        if (name != "name") {
            return false;
        }

        const auto inputName = parsing::sv_next_split(input);
        for (auto& option : s_uciOptions) {
            if (option.name == inputName) {
                const auto value = parsing::sv_next_split(input);
                if (value != "value") {
                    return false;
                }

                const auto inputValue = parsing::sv_next_split(input).value_or(input);
                return ucioption::handleInput(option, inputValue);
            }
        }

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
        } else if (command == "options") {
            for (const auto& option : s_uciOptions) {
                ucioption::printDebug(option);
            }
        } else if (command == "clear") {
            s_evaluator.reset();
            engine::TtHashTable::clear();
        } else if (command == "syzygy") {
            int32_t score = 0;
            const auto wdl = syzygy::probeWdl(s_board, score);
            fmt::println("wdl: {}, score: {}, table size: {}", wdl, score, syzygy::tableSize());
            if (!(wdl == syzygy::WdlResultFailed || wdl == syzygy::WdlResultTableNotActive))
                syzygy::printDtzDebug(s_board);
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

    static bool handleAuthors()
    {
        fmt::println("{}", s_meltdownAuthors);
        return true;
    }

    static bool handleBench(std::string_view args)
    {
        const auto depth = parsing::to_number(args);
        if (depth.has_value()) {
            engine::Bench::run(*depth);
        } else {
            engine::Bench::run();
        }

        return true;
    }

    static bool handleVersion()
    {
        fmt::print("Version:     {}\n"
                   "Build hash:  {}\n"
                   "Build type:  {}\n"
                   "Builtin:     {}\n\n",
            s_meltdownVersion, s_meltdownBuildHash, s_meltdownBuildType, s_meltdownBuiltinFeature);
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
                   "debug options       :  print all options\n"
                   "debug syzygy        :  run syzygy evaluation on current position\n"
                   "bench <depth>       :  run a bench test - depth is optional\n"
                   "authors             :  print author information\n"
                   "version             :  print version information\n"
                   "quit                :  stop the engine\n\n");

        return true;
    }

    static inline bool s_isRunning = false;
    static inline BitBoard s_board {};
    static inline evaluation::Evaluator s_evaluator;
    constexpr static inline std::size_t s_inputBufferSize { 2048 };

    /* options used in the UCI handler */
    static inline bool s_ponderingEnabled { false };

    /* UCI options callbacks */
    static inline void syzygyCallback(std::string_view path)
    {
        syzygy::deinit();

        if (!path.empty()) {
            if (!syzygy::init(path)) {
                fmt::println("Invalid syzygy path: {}", path);
            }
        }
    }

    /* declare UCI options */
    static inline auto s_uciOptions = std::to_array<ucioption::UciOption>({
        ucioption::make<ucioption::check>("Ponder", false, [](bool enabled) { s_ponderingEnabled = enabled; }),
        ucioption::make<ucioption::string>("Syzygy", "<empty>", syzygyCallback),
        /* EXAMPLE: how to add with limits: */
        /* ucioption::make<ucioption::spin>("Threads", 1, ucioption::Limits { .min = 1, .max = 256 }, [](uint64_t val) {  }), */
    });
};

