#pragma once

#include "src/engine.h"
#include <chrono>
#include <iostream>

class Perft {
public:
    constexpr static inline void run(const Engine& engine, uint8_t depth)
    {
        reset();
        std::cout << std::format("*** Starting perft - depth {} ***\n", depth);

        using namespace std::chrono;
        const auto startTime = system_clock::now();

        if (depth == 0) {
            s_nodes = 1;
        } else {
            search(engine, depth - 1, true);
        }

        const auto endTime = system_clock::now();
        const auto timeDiff = duration_cast<milliseconds>(endTime - startTime).count();

        std::cout << "\nnodes: " << s_nodes;
        std::cout << "\ncaptures: " << s_captures;
        std::cout << "\ncastles: " << s_castles;
        std::cout << "\nchecks: " << s_checks;
        std::cout << "\ntime: " << timeDiff << "ms";

        std::cout << std::endl;
    }

private:
    constexpr static inline void search(const Engine& engine, uint8_t depth, bool printMove = false)
    {
        const auto allMoves = engine.getAllMoves();

        if (depth == 0) {
            for (const auto& move : allMoves.getMoves()) {
                Engine newEngine = engine;
                newEngine.performMove(move);

                if (newEngine.isKingAttacked(engine.getCurrentPlayer())) {
                    // invalid move
                    continue;
                }

                if (newEngine.isKingAttacked()) {
                    s_checks++;
                }

                if (magic_enum::enum_flags_test(move.flags, movement::MoveFlags::Capture)) {
                    s_captures++;
                }

                if (magic_enum::enum_flags_test(move.flags, movement::MoveFlags::Castle)) {
                    s_castles++;
                }

                s_nodes++;
            }

            return;
        }

        for (const auto& move : allMoves.getMoves()) {
            Engine newEngine = engine;
            newEngine.performMove(move);

            if (newEngine.isKingAttacked()) {
                // invalid move
                continue;
            }

            search(newEngine, depth - 1);

            if (printMove) {
                std::cout << move.toString() << " " << s_nodes - s_prevNodes << std::endl;
                s_prevNodes = s_nodes;
            }
        }
    }

    static inline void reset()
    {
        s_nodes = 0;
        s_captures = 0;
        s_castles = 0;
        s_checks = 0;
        s_prevNodes = 0;
    }

    static inline uint64_t s_nodes {};
    static inline uint64_t s_captures {};
    static inline uint64_t s_castles {};
    static inline uint64_t s_checks {};
    static inline uint64_t s_prevNodes {};
};
