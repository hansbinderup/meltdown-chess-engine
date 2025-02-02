#pragma once

#include "src/board_defs.h"
#include <array>
#include <cstdint>
#include <limits>

namespace positioning {

namespace {

constexpr auto s_pawnTable = std::to_array<int16_t>(
    { 0, 5, 5, 0, 5, 10, 50, 0,
        0, 10, -5, 0, 5, 10, 50, 0,
        0, 5, -5, 10, 20, 35, 50, 0,
        5, 5, 0, 25, 35, 50, 50, 5,
        5, 10, 10, 20, 40, 50, 50, 5,
        10, 15, 20, 30, 50, 50, 50, 10,
        50, 50, 50, 50, 50, 50, 50, 50,
        0, 0, 0, 0, 0, 0, 0, 0 });

constexpr auto s_knightTable = std::to_array<int16_t>(
    { -50, -40, -30, -30, -30, -30, -40, -50,
        -40, -20, 0, 0, 0, 0, -20, -40,
        -30, 0, 10, 15, 15, 10, 0, -30,
        -30, 5, 15, 20, 20, 15, 5, -30,
        -30, 0, 15, 20, 20, 15, 0, -30,
        -30, 5, 10, 15, 15, 10, 5, -30,
        -40, -20, 0, 5, 5, 0, -20, -40,
        -50, -40, -30, -30, -30, -30, -40, -50 });

constexpr auto s_bishopTable = std::to_array<int16_t>(
    { -20, -10, -10, -10, -10, -10, -10, -20,
        -10, 0, 0, 0, 0, 0, 0, -10,
        -10, 0, 5, 10, 10, 5, 0, -10,
        -10, 5, 5, 10, 10, 5, 5, -10,
        -10, 0, 10, 10, 10, 10, 0, -10,
        -10, 10, 10, 10, 10, 10, 10, -10,
        -10, 5, 0, 0, 0, 0, 5, -10,
        -20, -10, -10, -10, -10, -10, -10, -20 });

constexpr auto s_rookTable = std::to_array<int16_t>(
    { 0, 0, 0, 5, 5, 0, 0, 0,
        -5, 0, 0, 0, 0, 0, 0, -5,
        -5, 0, 0, 0, 0, 0, 0, -5,
        -5, 0, 0, 0, 0, 0, 0, -5,
        -5, 0, 0, 0, 0, 0, 0, -5,
        -5, 0, 0, 0, 0, 0, 0, -5,
        5, 10, 10, 10, 10, 10, 10, 5,
        0, 0, 0, 5, 5, 0, 0, 0 });

constexpr auto s_queenTable = std::to_array<int16_t>(
    { -20, -10, -10, -5, -5, -10, -10, -20,
        -10, 0, 0, 0, 0, 0, 0, -10,
        -10, 0, 5, 5, 5, 5, 0, -10,
        -5, 0, 5, 5, 5, 5, 0, -5,
        0, 0, 5, 5, 5, 5, 0, -5,
        -10, 5, 5, 5, 5, 5, 0, -10,
        -10, 0, 5, 0, 0, 0, 0, -10,
        -20, -10, -10, -5, -5, -10, -10, -20 });

constexpr auto s_kingTable = std::to_array<int16_t>(
    { -30, -40, -40, -50, -50, -40, -40, -30,
        -40, -50, -50, -60, -60, -50, -50, -40,
        -40, -50, -50, -60, -60, -50, -50, -40,
        -50, -60, -60, -70, -70, -60, -60, -50,
        -60, -70, -70, -80, -80, -70, -70, -60,
        -60, -70, -70, -80, -80, -70, -70, -60,
        -50, -60, -60, -70, -70, -60, -60, -50,
        -30, -40, -40, -50, -50, -40, -40, -30 });

}

static inline int16_t calculatePawnScore(uint64_t pawns, Player player)
{
    int16_t score = 0;
    while (pawns) {
        int square = std::countr_zero(pawns); // Find the lowest set bit
        score += (player == Player::White ? s_pawnTable[square] : s_pawnTable[63 - square]); // Flip for Black
        pawns &= (pawns - 1); // Remove the lowest set bit
    }
    return score;
}

static inline int16_t calculateRookScore(uint64_t rooks, Player player)
{
    int16_t score = 0;
    while (rooks) {
        int square = std::countr_zero(rooks); // Find the lowest set bit
        score += (player == Player::White ? s_rookTable[square] : s_rookTable[63 - square]); // Flip for Black
        rooks &= (rooks - 1); // Remove the lowest set bit
    }
    return score;
}

static inline int16_t calculateBishopScore(uint64_t bishops, Player player)
{
    int16_t score = 0;
    while (bishops) {
        int square = std::countr_zero(bishops); // Find the lowest set bit
        score += (player == Player::White ? s_bishopTable[square] : s_bishopTable[63 - square]); // Flip for Black
        bishops &= (bishops - 1); // Remove the lowest set bit
    }
    return score;
}

static inline int16_t calculateKnightScore(uint64_t knights, Player player)
{
    int16_t score = 0;
    while (knights) {
        int square = std::countr_zero(knights); // Find the lowest set bit
        score += (player == Player::White ? s_knightTable[square] : s_knightTable[63 - square]); // Flip for Black
        knights &= (knights - 1); // Remove the lowest set bit
    }
    return score;
}

static inline int16_t calculateQueenScore(uint64_t queens, Player player)
{
    int16_t score = 0;
    while (queens) {
        int square = std::countr_zero(queens); // Find the lowest set bit
        score += (player == Player::White ? s_queenTable[square] : s_queenTable[63 - square]); // Flip for Black
        queens &= (queens - 1); // Remove the lowest set bit
    }
    return score;
}

static inline int16_t calculateKingScore(uint64_t king, Player player)
{
    if (king == 0) {
        return std::numeric_limits<int16_t>::min();
    }

    int16_t score = 0;

    int square = std::countr_zero(king); // Find the lowest set bit
    score += (player == Player::White ? s_kingTable[square] : s_kingTable[63 - square]); // Flip for Black
    king &= (king - 1); // Remove the lowest set bit

    return score;
}

constexpr uint8_t s_earlyMovementClassification { 7 };
constexpr int8_t s_earlyKingPenalty { 15 };
constexpr int8_t s_earlyQueenPenalty { 10 };

}
