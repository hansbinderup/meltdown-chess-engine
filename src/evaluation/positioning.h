#pragma once

#include "src/bit_board.h"
#include "src/board_defs.h"
#include <array>
#include <cstdint>

#include "src/evaluation/position_tables.h"
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

template<Player player>
constexpr static inline int32_t getKnightScore(const uint64_t knights)
{
    int32_t score = 0;
    uint64_t pieces = knights; // iterate each knight and compare to all the knights - hence the copy

    while (pieces) {
        int square = std::countr_zero(pieces); // Find the lowest set bit
        pieces &= (pieces - 1); // Remove the lowest set bit

        if constexpr (player == PlayerWhite) {
            score += s_whiteKnightTable[square];
        } else {
            score += s_blackKnightTable[square];
        }
    }

    return score;
}

template<Player player>
constexpr static inline int32_t getBishopScore(const BitBoard& board, const uint64_t bishops)
{
    int32_t score = 0;
    uint64_t pieces = bishops; // iterate each bishop and compare to all the bishops - hence the copy

    while (pieces) {
        int square = std::countr_zero(pieces); // Find the lowest set bit
        pieces &= (pieces - 1); // Remove the lowest set bit

        const uint64_t moves = movement::getBishopAttacks(square, board.occupation[Both]);
        score += std::popcount(moves) * s_bishopMobilityScore;

        if constexpr (player == PlayerWhite) {
            score += s_whiteBishopTable[square];
        } else {
            score += s_blackBishopTable[square];
        }
    }

    return score;
}

template<Player player>
constexpr static inline int32_t getRookScore(const BitBoard& board, const uint64_t rooks)
{
    int32_t score = 0;
    uint64_t pieces = rooks; // iterate each rook and compare to all the rooks - hence the copy

    while (pieces) {
        int square = std::countr_zero(pieces); // Find the lowest set bit
        pieces &= (pieces - 1); // Remove the lowest set bit

        const uint64_t whitePawns = board.pieces[WhitePawn];
        const uint64_t blackPawns = board.pieces[BlackPawn];

        if (((whitePawns | blackPawns) & s_fileMaskTable[square]) == 0)
            score += s_openFileScore;

        if constexpr (player == PlayerWhite) {
            score += s_whiteRookTable[square];
            if ((whitePawns & s_fileMaskTable[square]) == 0)
                score += s_semiOpenFileScore;

        } else {
            score += s_blackRookTable[square];
            if ((blackPawns & s_isolationMaskTable[square]) == 0)
                score += s_semiOpenFileScore;
        }
    }

    return score;
}

template<Player player>
constexpr static inline int32_t getQueenScore(const BitBoard& board, const uint64_t queens)
{
    int32_t score = 0;
    uint64_t pieces = queens; // iterate each queen and compare to all the queens - hence the copy

    while (pieces) {
        int square = std::countr_zero(pieces); // Find the lowest set bit
        pieces &= (pieces - 1); // Remove the lowest set bit

        const uint64_t moves
            = movement::getBishopAttacks(square, board.occupation[Both])
            | movement::getRookAttacks(square, board.occupation[Both]);

        score += std::popcount(moves) * s_queenMobilityScore;

        if constexpr (player == PlayerWhite) {
            score += s_whiteQueenTable[square];
        } else {
            score += s_blackQueenTable[square];
        }
    }

    return score;
}

template<Player player>
constexpr static inline int32_t getKingScore(const BitBoard& board, const uint64_t king)
{
    int32_t score = 0;
    uint64_t pieces = king; // we're gonna clear the mask so keep the copy even though it's only one king

    while (pieces) {
        int square = std::countr_zero(pieces); // Find the lowest set bit
        pieces &= (pieces - 1); // Remove the lowest set bit

        const uint64_t whitePawns = board.pieces[WhitePawn];
        const uint64_t blackPawns = board.pieces[BlackPawn];

        /* king will get a penalty for semi/open files */
        if (((whitePawns | blackPawns) & s_fileMaskTable[square]) == 0)
            score -= s_openFileScore;

        if constexpr (player == PlayerWhite) {
            score += s_whiteKingTable[square];
            if ((whitePawns & s_fileMaskTable[square]) == 0)
                score -= s_semiOpenFileScore;

            const uint64_t kingShields = movement::getKingAttacks(square) & board.occupation[PlayerWhite];
            score += std::popcount(kingShields) * s_kingShieldScore;
        } else {
            score += s_blackKingTable[pos];
            if ((blackPawns & s_fileMaskTable[pos]) == 0)
                score -= s_semiOpenFileScore;

            const uint64_t kingShields = movement::getKingAttacks(square) & board.occupation[PlayerBlack];
            score += std::popcount(kingShields) * s_kingShieldScore;
        }
    }

    return score;
}

}

}
