#pragma once

#include "src/bit_board.h"
#include "src/board_defs.h"
#include <array>
#include <cstdint>

#include "src/evaluation/position_tables.h"
#include "src/helpers/bit_operations.h"
#include "src/movement/bishops.h"
#include "src/movement/kings.h"
#include "src/movement/rooks.h"

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

constexpr int32_t s_bishopMobilityScore { 3 };
constexpr int32_t s_queenMobilityScore { 1 }; /* TODO: make game phase dependent */
constexpr int32_t s_kingShieldScore { 5 };

}

namespace position {

template<Player player>
constexpr static inline int32_t getPawnScore(const BitBoard& board, const uint64_t pawns)
{
    int32_t score = 0;

    helper::bitIterate(pawns, [&](BoardPosition pos) {
        const auto doubledPawns = std::popcount(pawns & s_fileMaskTable[pos]);
        if (doubledPawns > 1)
            score += doubledPawns * s_doublePawnPenalty;

        if ((pawns & s_isolationMaskTable[pos]) == 0)
            score += s_isolatedPawnPenalty;

        if constexpr (player == PlayerWhite) {
            score += s_whitePawnTable[pos];
            if ((board.pieces[BlackPawn] & s_whitePassedPawnMaskTable[pos]) == 0) {
                const uint8_t row = (pos / 8);
                score += s_passedPawnBonus[row];
            }
        } else {
            score += s_blackPawnTable[pos];
            if ((board.pieces[WhitePawn] & s_blackPassedPawnMaskTable[pos]) == 0) {
                const uint8_t row = (pos / 8);
                score += s_passedPawnBonus[7 - row]; /* invert table for black */
            }
        }
    });

    return score;
}

template<Player player>
constexpr static inline int32_t getKnightScore(const uint64_t knights)
{
    int32_t score = 0;

    helper::bitIterate(knights, [&score](BoardPosition pos) {
        if constexpr (player == PlayerWhite) {
            score += s_whiteKnightTable[pos];
        } else {
            score += s_blackKnightTable[pos];
        }
    });

    return score;
}

template<Player player>
constexpr static inline int32_t getBishopScore(const BitBoard& board, const uint64_t bishops)
{
    int32_t score = 0;

    helper::bitIterate(bishops, [&score, &board](BoardPosition pos) {
        const uint64_t moves = movement::getBishopAttacks(pos, board.occupation[Both]);
        score += std::popcount(moves) * s_bishopMobilityScore;

        if constexpr (player == PlayerWhite) {
            score += s_whiteBishopTable[pos];
        } else {
            score += s_blackBishopTable[pos];
        }
    });

    return score;
}

template<Player player>
constexpr static inline int32_t getRookScore(const BitBoard& board, const uint64_t rooks)
{
    int32_t score = 0;

    const uint64_t whitePawns = board.pieces[WhitePawn];
    const uint64_t blackPawns = board.pieces[BlackPawn];

    helper::bitIterate(rooks, [&](BoardPosition pos) {
        if (((whitePawns | blackPawns) & s_fileMaskTable[pos]) == 0)
            score += s_openFileScore;

        if constexpr (player == PlayerWhite) {
            score += s_whiteRookTable[pos];
            if ((whitePawns & s_fileMaskTable[pos]) == 0)
                score += s_semiOpenFileScore;

        } else {
            score += s_blackRookTable[pos];
            if ((blackPawns & s_isolationMaskTable[pos]) == 0)
                score += s_semiOpenFileScore;
        }
    });

    return score;
}

template<Player player>
constexpr static inline int32_t getQueenScore(const BitBoard& board, const uint64_t queens)
{
    int32_t score = 0;

    helper::bitIterate(queens, [&board, &score](BoardPosition pos) {
        const uint64_t moves
            = movement::getBishopAttacks(pos, board.occupation[Both])
            | movement::getRookAttacks(pos, board.occupation[Both]);

        score += std::popcount(moves) * s_queenMobilityScore;

        if constexpr (player == PlayerWhite) {
            score += s_whiteQueenTable[pos];
        } else {
            score += s_blackQueenTable[pos];
        }
    });

    return score;
}

template<Player player>
constexpr static inline int32_t getKingScore(const BitBoard& board, const uint64_t king)
{
    int32_t score = 0;

    const uint64_t whitePawns = board.pieces[WhitePawn];
    const uint64_t blackPawns = board.pieces[BlackPawn];

    helper::bitIterate(king, [&](BoardPosition pos) {
        /* king will get a penalty for semi/open files */
        if (((whitePawns | blackPawns) & s_fileMaskTable[pos]) == 0)
            score -= s_openFileScore;

        if constexpr (player == PlayerWhite) {
            score += s_whiteKingTable[pos];
            if ((whitePawns & s_fileMaskTable[pos]) == 0)
                score -= s_semiOpenFileScore;

            const uint64_t kingShields = movement::getKingAttacks(pos) & board.occupation[PlayerWhite];
            score += std::popcount(kingShields) * s_kingShieldScore;
        } else {
            score += s_blackKingTable[pos];
            if ((blackPawns & s_fileMaskTable[pos]) == 0)
                score -= s_semiOpenFileScore;

            const uint64_t kingShields = movement::getKingAttacks(pos) & board.occupation[PlayerBlack];
            score += std::popcount(kingShields) * s_kingShieldScore;
        }
    });

    return score;
}

}

}
