#pragma once

#include "core/board_defs.h"
#include "evaluation/evaluator.h"
#include "parsing/fen_parser.h"
#include "parsing/input_parsing.h"
#include "spsa/parameters.h"
#include "tools/bench.h"
#include "tools/perft.h"

#include "interface/uci_options.h"
#include "syzygy/syzygy.h"
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
        core::TranspositionTable::setSizeMb(s_defaultTtSizeMb);
        s_evaluator.reset();

        startInputThread();

        /* always release resources */
        s_evaluator.stop();
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
        } else if (command == "pprint") {
            return handlePrettyPrint(args);
        } else if (command == "spsa") {
            return handleSpsa();
        } else if (command == "version") {
            return handleVersion();
        } else if (command == "quit" || command == "exit") {
            return handleQuit();
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
        fmt::println("id name Meltdown {}\n"
                     "id author Hans Binderup",
            s_meltdownVersion);

        for (const auto& option : s_uciOptions) {
            ucioption::printInfo(option);
        }

#ifdef SPSA
        for (const auto& option : spsa::uciOptions) {
            ucioption::printInfo(option);
        }
#endif

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
            while (true) {
                const auto moveSv = parsing::sv_next_split(sv);
                const auto move = parsing::moveFromString(s_board, moveSv.value_or(sv));

                if (move.has_value()) {
                    s_evaluator.updateRepetition(s_board.hash);
                    s_board = core::performMove(s_board, move.value());
                } else {
                    break;
                }
            }
        };

        if (command == "startpos") {
            s_board.reset();

            s_evaluator.reset();

            const auto subCommand = parsing::sv_next_split(args);
            if (subCommand == "moves") {
                iterateMovesFnc(args);
            }
        } else if (command == "fen") {
            const auto board = parsing::FenParser::parse(args);
            if (board.has_value()) {
                s_board = std::move(board.value());

                s_evaluator.reset();

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
        core::TranspositionTable::clear();

        return true;
    }

    static bool handlePonderhit()
    {
        s_evaluator.onPonderHit(s_board);

        return true;
    }

    static bool handleGo(std::string_view args)
    {
        std::optional<uint8_t> depth;

        s_evaluator.resetTiming();

        bool ponder = false;

        while (const auto setting = parsing::sv_next_split(args)) {
            /* single word settings first */
            if (setting == "ponder") {
                ponder = true;
                continue;
            }

            /* settings with values */
            const auto value = parsing::sv_next_split(args);
            const auto valNum = parsing::to_number(value.value_or(args));

            if (setting == "wtime") {
                TimeManager::setWhiteTime(valNum.value_or(0));
            } else if (setting == "btime") {
                TimeManager::setBlackTime(valNum.value_or(0));
            } else if (setting == "movestogo") {
                TimeManager::setMovesToGo(valNum.value_or(0));
            } else if (setting == "movetime") {
                TimeManager::setMoveTime(valNum.value_or(0));
            } else if (setting == "winc") {
                TimeManager::setWhiteMoveInc(valNum.value_or(0));
            } else if (setting == "binc") {
                TimeManager::setBlackMoveInc(valNum.value_or(0));
            } else if (setting == "depth") {
                depth = valNum;
            }
        }

        if (ponder) {
            s_evaluator.startPondering(s_board);
        } else {
            s_evaluator.getBestMoveAsync(s_board, depth);
        }

        return true;
    }

    static bool handleStop()
    {
        s_evaluator.stop();

        return true;
    }

    static bool handleQuit()
    {
        s_evaluator.kill();
        s_isRunning = false;

        return true;
    }

    static bool handleSetOption(std::string_view input)
    {
        const auto name = parsing::sv_next_split(input);
        if (name != "name") {
            return false;
        }

        const auto inputName = parsing::sv_next_split(input);
        const auto handleOptionFnc = [&](ucioption::UciOption& option) {
            if (option.name == inputName) {
                const auto value = parsing::sv_next_split(input);
                if (value != "value") {
                    return;
                }

                const auto inputValue = parsing::sv_next_split(input).value_or(input);
                ucioption::handleInput(option, inputValue);
            }
        };

        for (auto& option : s_uciOptions) {
            handleOptionFnc(option);
        }

#ifdef SPSA
        for (auto& option : spsa::uciOptions) {
            handleOptionFnc(option);
        }
#endif

        return true;
    }

    static bool handleDebug(std::string_view input)
    {
        auto [command, args] = parsing::split_sv_by_space(input);
        if (command == "position") {
            core::printPositionDebug(s_board);
        } else if (command == "eval") {
            const auto depth = parsing::to_number(args);
            s_evaluator.printEvaluation(s_board, depth);
        } else if (command == "options") {
            for (const auto& option : s_uciOptions) {
                ucioption::printDebug(option);
            }
#ifdef SPSA
            for (const auto& option : spsa::uciOptions) {
                ucioption::printDebug(option);
            }
#endif
        } else if (command == "clear") {
            s_evaluator.reset();
            core::TranspositionTable::clear();
        } else if (command == "syzygy") {
            const auto wdl = syzygy::probeWdl(s_board);
            fmt::println("wdl: {}, table size: {}", wdl, syzygy::tableSize());
            if (!(wdl == syzygy::WdlResultFailed || wdl == syzygy::WdlResultTableNotActive))
                syzygy::printDtzDebug(s_board);
        }

        return true;
    }

    static bool handlePerft(std::string_view args)
    {
        const auto depth = parsing::to_number(args);
        if (depth.has_value()) {
            tools::Perft::run(s_board, depth.value());
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
            tools::Bench::run(s_evaluator, *depth);
        } else {
            tools::Bench::run(s_evaluator);
        }

        return true;
    }

    static bool handlePrettyPrint(std::string_view args)
    {
        const auto enabled = parsing::sv_next_split(args).value_or(args);
        if (enabled == "on") {
            interface::setPrettyPrintEnabled(true);
        } else if (enabled == "off") {
            interface::setPrettyPrintEnabled(false);
        }

        return true;
    }

    static bool handleSpsa()
    {
#ifdef SPSA
        fmt::println("{}", spsa::inputsPrint);
#else
        fmt::println("Meltdown supports SPSA tuning\n"
                     "The feature is currently disabled\n"
                     "For more details, see: src/spsa/README.md");
#endif

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
                   "pprint <on/off>     :  enable/disable pretty printing\n"
                   "spsa                :  print spsa inputs\n"
                   "authors             :  print author information\n"
                   "version             :  print version information\n"
                   "quit                :  stop the engine\n\n");

        return true;
    }

    static inline bool s_isRunning = false;
    static inline BitBoard s_board {};
    static inline evaluation::Evaluator s_evaluator;

    /* 1024 bytes -> 200~ plys
     * aim to support around 1000 plys of moves + a buffer to be sure */
    constexpr static inline std::size_t s_inputBufferSize { 1024 * 6 };

    /* UCI options callbacks */
    static inline void syzygyPathCallback(std::string_view path)
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
        ucioption::make<ucioption::check>("Ponder", false, [](bool enabled) { s_evaluator.setPondering(enabled); }),
        ucioption::make<ucioption::string>("SyzygyPath", "<empty>", syzygyPathCallback),
        ucioption::make<ucioption::spin>("SyzygyProbeLimit", 0, ucioption::Limits { .min = 0, .max = 7 }, [](int64_t val) {
            /* FIXME: we don't currently use the probe limit - we use it dynamically based on table depths
             * we could provide this option and allow overwrite, but for now just act like we're doing that to
             * mute warnings from OpenBench */
            std::ignore = val;
        }),
        ucioption::make<ucioption::spin>("Hash", s_defaultTtSizeMb, ucioption::Limits { .min = 1, .max = 1024 }, [](int64_t val) { core::TranspositionTable::setSizeMb(val); }),
        ucioption::make<ucioption::spin>("Threads", 1, ucioption::Limits { .min = 1, .max = s_maxThreads }, [](int64_t val) {
            s_evaluator.resizeSearchers(val);
        }),
        ucioption::make<ucioption::spin>("MoveOverhead", s_defaultMoveOverhead.count(), ucioption::Limits { .min = 0, .max = 10000 }, [](int64_t val) {
            TimeManager::setMoveOverhead(val);
        }),
    });
};
