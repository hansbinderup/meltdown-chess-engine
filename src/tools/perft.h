#pragma once

#include "core/bit_board.h"
#include "core/move_handling.h"
#include <chrono>

#include "utils/formatters.h"

namespace tools {

class Perft {
public:
    constexpr static void run(const BitBoard& board, uint8_t depth)
    {
        reset();
        fmt::print("*** Starting perft - depth {} ***\n", depth);

        using namespace std::chrono;
        const auto startTime = steady_clock::now();

        if (depth == 0) {
            s_nodes = 1;
        } else {
            search(board, depth - 1, true);
        }

        const auto endTime = steady_clock::now();
        /* time in seconds reflected as a double */
        const auto timeDiff = duration_cast<duration<double>>(endTime - startTime).count();

        fmt::print("\n*** result ***\n"
                   "nodes:       {}\n"
                   "captures:    {}\n"
                   "castles:     {}\n"
                   "enpessants:  {}\n"
                   "promotions:  {}\n"
                   "checks:      {}\n"
                   "checkmates:  {}\n"
                   "nps:         {:.0f}\n"
                   "time:        {:.2f}ms\n",
            s_nodes, s_captures, s_castles, s_enPessants, s_promotions, s_checks, s_checkMates, s_nodes / timeDiff, timeDiff * 1000);
    }

private:
    constexpr static void search(const BitBoard& board, uint8_t depth, bool printMove = false)
    {
        movegen::ValidMoves moves;
        core::getAllMoves<movegen::MovePseudoLegal>(board, moves);
        uint64_t legalMoves = 0;

        if (depth == 0) {
            for (const auto& move : moves) {
                auto newBoard = core::performMove(board, move, s_hash);
                if (core::isKingAttacked(newBoard, board.player)) {
                    // invalid move
                    continue;
                }

                legalMoves++;

                if (core::isKingAttacked(newBoard)) {
                    s_checks++;
                }

                if (move.isCapture()) {
                    s_captures++;
                }

                if (move.isCastleMove()) {
                    s_castles++;
                }

                if (move.takeEnPessant()) {
                    s_enPessants++;
                }

                if (move.isPromotionMove())
                    s_promotions++;

                s_nodes++;
            }

            if (legalMoves == 0) {
                s_checkMates++;
            }

            return;
        }

        for (const auto& move : moves) {
            auto newBoard = core::performMove(board, move, s_hash);

            if (core::isKingAttacked(newBoard, board.player)) {
                // invalid move
                continue;
            }

            legalMoves++;

            search(newBoard, depth - 1);

            if (printMove) {
                fmt::println("{}: {}", move, s_nodes - s_prevNodes);
                s_prevNodes = s_nodes;
            }
        }

        if (legalMoves == 0) {
            s_checkMates++;
        }
    }

    void static reset()
    {
        s_nodes = 0;
        s_captures = 0;
        s_castles = 0;
        s_checks = 0;
        s_checkMates = 0;
        s_enPessants = 0;
        s_promotions = 0;
        s_prevNodes = 0;
    }

    static inline uint64_t s_nodes {};
    static inline uint64_t s_captures {};
    static inline uint64_t s_castles {};
    static inline uint64_t s_checks {};
    static inline uint64_t s_checkMates {};
    static inline uint64_t s_enPessants {};
    static inline uint64_t s_promotions {};
    static inline uint64_t s_prevNodes {};
    static inline uint64_t s_hash {};
};

}
