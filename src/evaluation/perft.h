#pragma once

#include "src/bit_board.h"
#include "src/engine/move_handling.h"
#include <chrono>
#include <iostream>

class Perft {
public:
    constexpr static inline void run(const BitBoard& board, uint8_t depth)
    {
        reset();
        std::cout << std::format("*** Starting perft - depth {} ***\n", depth);

        using namespace std::chrono;
        const auto startTime = system_clock::now();

        if (depth == 0) {
            s_nodes = 1;
        } else {
            search(board, depth - 1, true);
        }

        const auto endTime = system_clock::now();
        const auto timeDiff = duration_cast<milliseconds>(endTime - startTime).count();

        std::cout << "\nnodes: " << s_nodes;
        std::cout << "\ncaptures: " << s_captures;
        std::cout << "\ncastles: " << s_castles;
        std::cout << "\nchecks: " << s_checks;
        std::cout << "\ncheckMates: " << s_checkMates;
        std::cout << "\ntime: " << timeDiff << "ms";

        std::cout << std::endl;
    }

private:
    constexpr static inline void search(const BitBoard& board, uint8_t depth, bool printMove = false)
    {
        const auto allMoves = engine::getAllMoves(board);
        uint64_t legalMoves = 0;

        if (depth == 0) {
            for (const auto& move : allMoves.getMoves()) {
                auto newBoard = engine::performMove(board, move);
                if (engine::isKingAttacked(newBoard, board.player)) {
                    // invalid move
                    continue;
                }

                legalMoves++;

                if (engine::isKingAttacked(newBoard)) {
                    s_checks++;
                }

                if (move.isCapture()) {
                    s_captures++;
                }

                if (move.isCastleMove()) {
                    s_castles++;
                }

                s_nodes++;
            }

            if (legalMoves == 0) {
                s_checkMates++;
            }

            return;
        }

        for (const auto& move : allMoves.getMoves()) {
            auto newBoard = engine::performMove(board, move);

            if (engine::isKingAttacked(newBoard)) {
                // invalid move
                continue;
            }

            legalMoves++;

            search(newBoard, depth - 1);

            if (printMove) {
                std::cout << move.toString().data() << " " << s_nodes - s_prevNodes << std::endl;
                s_prevNodes = s_nodes;
            }
        }

        if (legalMoves == 0) {
            s_checkMates++;
        }
    }

    static inline void reset()
    {
        s_nodes = 0;
        s_captures = 0;
        s_castles = 0;
        s_checks = 0;
        s_checkMates = 0;
        s_prevNodes = 0;
    }

    static inline uint64_t s_nodes {};
    static inline uint64_t s_captures {};
    static inline uint64_t s_castles {};
    static inline uint64_t s_checks {};
    static inline uint64_t s_checkMates {};
    static inline uint64_t s_prevNodes {};
};
