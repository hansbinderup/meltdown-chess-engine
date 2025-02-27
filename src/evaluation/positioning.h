#pragma once

#include "src/board_defs.h"
#include <array>
#include <cstdint>

/*
 * Based on the tables from:
 * https://www.chessprogramming.org/Simplified_Evaluation_Function
 */

namespace evaluation {

namespace {

constexpr auto s_whitePawnTable = std::to_array<int16_t>(
    { 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, -10, -10, 0, 0, 0,
        0, 0, 0, 5, 5, 0, 0, 0,
        5, 5, 10, 20, 20, 5, 5, 5,
        10, 10, 10, 20, 20, 10, 10, 10,
        20, 20, 20, 30, 30, 30, 20, 20,
        30, 30, 30, 40, 40, 30, 30, 30,
        90, 90, 90, 90, 90, 90, 90, 90 });

constexpr auto s_whiteKnightTable = std::to_array<int16_t>(
    { -5, -10, 0, 0, 0, 0, -10, -5,
        -5, 0, 0, 0, 0, 0, 0, -5,
        -5, 5, 20, 10, 10, 20, 5, -5,
        -5, 10, 20, 30, 30, 20, 10, -5,
        -5, 10, 20, 30, 30, 20, 10, -5,
        -5, 5, 20, 20, 20, 20, 5, -5,
        -5, 0, 0, 10, 10, 0, 0, -5,
        -5, 0, 0, 0, 0, 0, 0, -5 });

constexpr auto s_whiteBishopTable = std::to_array<int16_t>(
    { 0, 0, -10, 0, 0, -10, 0, 0,
        0, 30, 0, 0, 0, 0, 30, 0,
        0, 10, 0, 0, 0, 0, 10, 0,
        0, 0, 10, 20, 20, 10, 0, 0,
        0, 0, 10, 20, 20, 10, 0, 0,
        0, 0, 0, 10, 10, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0 });

constexpr auto s_whiteRookTable = std::to_array<int16_t>(
    { 0, 0, 0, 20, 20, 0, 0, 0,
        0, 0, 10, 20, 20, 10, 0, 0,
        0, 0, 10, 20, 20, 10, 0, 0,
        0, 0, 10, 20, 20, 10, 0, 0,
        0, 0, 10, 20, 20, 10, 0, 0,
        0, 0, 10, 20, 20, 10, 0, 0,
        50, 50, 50, 50, 50, 50, 50, 50,
        50, 50, 50, 50, 50, 50, 50, 50 });

constexpr auto s_whiteQueenTable = std::to_array<int16_t>(
    { -20, -10, -10, -5, -5, -10, -10, -20,
        -10, 0, 5, 0, 0, 0, 0, -10,
        -10, 5, 5, 5, 5, 5, 0, -10,
        0, 0, 5, 5, 5, 5, 0, -5,
        -5, 0, 5, 5, 5, 5, 0, -5,
        -10, 0, 5, 5, 5, 5, 0, -10,
        -10, 0, 0, 0, 0, 0, 0, -10,
        -20, -10, -10, -5, -5, -10, -10, -20 });

constexpr auto s_whiteKingTable = std::to_array<int16_t>(
    { 0, 0, 5, 0, -15, 0, 10, 0,
        0, 5, 5, -5, -5, 0, 5, 0,
        0, 0, 5, 10, 10, 5, 0, 0,
        0, 5, 10, 20, 20, 10, 5, 0,
        0, 5, 10, 20, 20, 10, 5, 0,
        0, 5, 5, 10, 10, 5, 5, 0,
        0, 0, 5, 5, 5, 5, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0 });

constexpr auto s_blackPawnTable = std::to_array<int16_t>(
    { 90, 90, 90, 90, 90, 90, 90, 90,
        30, 30, 30, 40, 40, 30, 30, 30,
        20, 20, 20, 30, 30, 30, 20, 20,
        10, 10, 10, 20, 20, 10, 10, 10,
        5, 5, 10, 20, 20, 5, 5, 5,
        0, 0, 0, 5, 5, 0, 0, 0,
        0, 0, 0, -10, -10, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0 });

constexpr auto s_blackKnightTable = std::to_array<int16_t>(
    { -5, 0, 0, 0, 0, 0, 0, -5,
        -5, 0, 0, 10, 10, 0, 0, -5,
        -5, 5, 20, 20, 20, 20, 5, -5,
        -5, 10, 20, 30, 30, 20, 10, -5,
        -5, 10, 20, 30, 30, 20, 10, -5,
        -5, 5, 20, 10, 10, 20, 5, -5,
        -5, 0, 0, 0, 0, 0, 0, -5,
        -5, -10, 0, 0, 0, 0, -10, -5 });

constexpr auto s_blackBishopTable = std::to_array<int16_t>(
    { 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 10, 10, 0, 0, 0,
        0, 0, 10, 20, 20, 10, 0, 0,
        0, 0, 10, 20, 20, 10, 0, 0,
        0, 10, 0, 0, 0, 0, 10, 0,
        0, 30, 0, 0, 0, 0, 30, 0,
        0, 0, -10, 0, 0, -10, 0, 0 });

constexpr auto s_blackRookTable = std::to_array<int16_t>(
    { 50, 50, 50, 50, 50, 50, 50, 50,
        50, 50, 50, 50, 50, 50, 50, 50,
        0, 0, 10, 20, 20, 10, 0, 0,
        0, 0, 10, 20, 20, 10, 0, 0,
        0, 0, 10, 20, 20, 10, 0, 0,
        0, 0, 10, 20, 20, 10, 0, 0,
        0, 0, 10, 20, 20, 10, 0, 0,
        0, 0, 0, 20, 20, 0, 0, 0 });

constexpr auto s_blackQueenTable = std::to_array<int16_t>(
    { -20, -10, -10, -5, -5, -10, -10, -20,
        -10, 0, 0, 0, 0, 0, 0, -10,
        -10, 0, 5, 5, 5, 5, 0, -10,
        -5, 0, 5, 5, 5, 5, 0, -5,
        0, 0, 5, 5, 5, 5, 0, -5,
        -10, 5, 5, 5, 5, 5, 0, -10,
        -10, 0, 5, 0, 0, 0, 0, -10,
        -20, -10, -10, -5, -5, -10, -10, -20 });

constexpr auto s_blackKingTable = std::to_array<int16_t>(
    { 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 5, 5, 5, 5, 0, 0,
        0, 5, 5, 10, 10, 5, 5, 0,
        0, 5, 10, 20, 20, 10, 5, 0,
        0, 5, 10, 20, 20, 10, 5, 0,
        0, 0, 5, 10, 10, 5, 0, 0,
        0, 5, 5, -5, -5, 0, 5, 0,
        0, 0, 5, 0, -15, 0, 10, 0 });

constexpr static inline auto& getTable(Piece type)
{
    switch (type) {
    case WhitePawn:
        return s_whitePawnTable;
    case WhiteKnight:
        return s_whiteKnightTable;
    case WhiteBishop:
        return s_whiteBishopTable;
    case WhiteRook:
        return s_whiteRookTable;
    case WhiteQueen:
        return s_whiteQueenTable;
    case WhiteKing:
        return s_whiteKingTable;
    case BlackPawn:
        return s_blackPawnTable;
    case BlackKnight:
        return s_blackKnightTable;
    case BlackBishop:
        return s_blackBishopTable;
    case BlackRook:
        return s_blackRookTable;
    case BlackQueen:
        return s_blackQueenTable;
    case BlackKing:
        return s_blackKingTable;
    }

    /* shouldn't happen but compiler might complain about it */
    return s_whitePawnTable;
}

}

constexpr static inline int16_t getPiecePositionScore(uint64_t piece, Piece type)
{
    const auto& table = getTable(type);

    int16_t score = 0;
    while (piece) {
        int square = std::countr_zero(piece); // Find the lowest set bit
        score += table[square];
        piece &= (piece - 1); // Remove the lowest set bit
    }

    return score;
}

}
