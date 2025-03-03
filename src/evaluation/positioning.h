#pragma once

#include "src/board_defs.h"
#include <array>
#include <cstdint>

#include "src/evaluation/position_tables.h"

/*
 * Based on the tables from:
 * https://www.chessprogramming.org/Simplified_Evaluation_Function
 */

namespace evaluation {

namespace {

constexpr int32_t s_doublePawnPenalty { -10 };
constexpr int32_t s_isolatedPawnPenalty { -10 };
constexpr auto s_passedPawnBonus = std::to_array<int32_t>({ 0, 10, 30, 50, 75, 100, 150, 200 });
constexpr int32_t s_semiOpenFileScore { 10 };
constexpr int32_t s_openFileScore { 15 };

}

constexpr static inline int32_t getPiecePositionScore(uint64_t piece, Piece type)
{
    const auto& table = getTable(type);

    int32_t score = 0;
    while (piece) {
        int square = std::countr_zero(piece); // Find the lowest set bit
        score += table[square];
        piece &= (piece - 1); // Remove the lowest set bit
    }

    return score;
}

template<Player player>
constexpr static inline int32_t getPawnScore(const uint64_t pawns)
{
    int32_t score = 0;
    uint64_t pieces = pawns; // iterate each pawn and compare to all the pawns - hence the copy

    while (pieces) {
        int square = std::countr_zero(pieces); // Find the lowest set bit
        pieces &= (pieces - 1); // Remove the lowest set bit

        const auto doubledPawns = std::popcount(pawns & s_fileMaskTable[square]);
        if (doubledPawns > 1)
            score += doubledPawns * s_doublePawnPenalty;

        if ((pawns & s_isolationMaskTable[square]) == 0)
            score += s_isolatedPawnPenalty;

        if constexpr (player == PlayerWhite) {
            score += s_whitePawnTable[square];
            if ((pawns & s_whitePassedPawnMaskTable[square]) == 0) {
                const uint8_t row = (square / 8);
                score += s_passedPawnBonus[row];
            }
        } else {
            score += s_blackPawnTable[square];
            if ((pawns & s_blackPassedPawnMaskTable[square]) == 0) {
                const uint8_t row = (square / 8);
                score += s_passedPawnBonus[7 - row]; /* invert table for black */
            }
        }
    }

    return score;
}

}
