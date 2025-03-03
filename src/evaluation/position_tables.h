#pragma once

#include "magic_enum/magic_enum.hpp"
#include <src/board_defs.h>

namespace evaluation {

namespace {

/* created here manually just to be explicit */
constexpr uint64_t s_fileA = {
    1ULL << A1 | 1ULL << A2 | 1ULL << A3 | 1ULL << A4
    | 1ULL << A5 | 1ULL << A6 | 1ULL << A7 | 1ULL << A8
};

/* Bitmask of the entire row of a given position
 *
 * eg: table for A1-B1...H1:
 * -8- 0 0 0 0 0 0 0 0
 * -7- 0 0 0 0 0 0 0 0
 * -6- 0 0 0 0 0 0 0 0
 * -5- 0 0 0 0 0 0 0 0
 * -4- 0 0 0 0 0 0 0 0
 * -3- 0 0 0 0 0 0 0 0
 * -2- 0 0 0 0 0 0 0 0
 * -1- 1 1 1 1 1 1 1 1
 *     A B C D E F G H
 */
constexpr auto generateRowMaskTable()
{
    std::array<uint64_t, s_amountSquares> data;

    for (const auto pos : magic_enum::enum_values<BoardPosition>()) {
        const auto row = pos / 8;
        data[pos] = 0xffULL << (row * 8);
    }

    return data;
}

/* Bitmask of the entire file of a given position
 *
 * eg: table for F1-F2...F8:
 * -8- 0 0 0 0 0 1 0 0
 * -7- 0 0 0 0 0 1 0 0
 * -6- 0 0 0 0 0 1 0 0
 * -5- 0 0 0 0 0 1 0 0
 * -4- 0 0 0 0 0 1 0 0
 * -3- 0 0 0 0 0 1 0 0
 * -2- 0 0 0 0 0 1 0 0
 * -1- 0 0 0 0 0 1 0 0
 *     A B C D E F G H
 */
constexpr auto generateFileMaskTable()
{
    std::array<uint64_t, s_amountSquares> data;

    for (const auto pos : magic_enum::enum_values<BoardPosition>()) {
        const auto file = pos % 8;
        data[pos] = s_fileA << file;
    }

    return data;
}

/* Bitmask to find if a piece is isolated on a given position
 * Ie. if a piece is on a neighbour file
 *
 * eg: table for F1-F2...F8:
 * -8- 0 0 0 0 1 0 1 0
 * -7- 0 0 0 0 1 0 1 0
 * -6- 0 0 0 0 1 0 1 0
 * -5- 0 0 0 0 1 0 1 0
 * -4- 0 0 0 0 1 0 1 0
 * -3- 0 0 0 0 1 0 1 0
 * -2- 0 0 0 0 1 0 1 0
 * -1- 0 0 0 0 1 0 1 0
 *     A B C D E F G H
 */
constexpr auto generateIsolationMaskTable()
{
    std::array<uint64_t, s_amountSquares> data {};

    for (const auto pos : magic_enum::enum_values<BoardPosition>()) {
        const auto file = pos % 8;

        /* H file only have a left side isolation mask
         * A file only have a right side isolation mask
         * Other files have isolation mask on both sides
         */
        if (file < 7)
            data[pos] |= s_fileA << (file + 1);

        if (file > 0)
            data[pos] |= s_fileA << (file - 1);
    }

    return data;
}

/* Bitmask to find passed pawns (White)
 * mask is the neighbor files + own file till the end of the board
 *
 * eg: table for F3:
 * -8- 0 0 0 0 1 1 1 0
 * -7- 0 0 0 0 1 1 1 0
 * -6- 0 0 0 0 1 1 1 0
 * -5- 0 0 0 0 1 1 1 0
 * -4- 0 0 0 0 1 1 1 0
 * -3- 0 0 0 0 0 0 0 0
 * -2- 0 0 0 0 0 0 0 0
 * -1- 0 0 0 0 0 0 0 0
 *     A B C D E F G H
 */
constexpr auto generateWhitePassedPawnMaskTable()
{
    std::array<uint64_t, s_amountSquares> data {};

    for (const auto pos : magic_enum::enum_values<BoardPosition>()) {
        uint8_t passedPos = pos + 8;
        const uint8_t file = passedPos % 8;
        uint64_t mask {};

        while (passedPos <= H8) {
            if (file < 7)
                mask |= 1ULL << (passedPos + 1);

            if (file > 0)
                mask |= 1ULL << (passedPos - 1);

            mask |= 1ULL << passedPos;

            passedPos += 8;
        }

        data[pos] = mask;
    }

    return data;
}

/* Bitmask to find passed pawns (White)
 * mask is the neighbor files + own file till the end of the board
 *
 * eg: table for C7:
 * -8- 0 0 0 0 0 0 0 0
 * -7- 0 0 0 0 0 0 0 0
 * -6- 0 1 1 1 0 0 0 0
 * -5- 0 1 1 1 0 0 0 0
 * -4- 0 1 1 1 0 0 0 0
 * -3- 0 1 1 1 0 0 0 0
 * -2- 0 1 1 1 0 0 0 0
 * -1- 0 1 1 1 0 0 0 0
 *     A B C D E F G H
 */
constexpr auto generateBlackPassedPawnMaskTable()
{
    std::array<uint64_t, s_amountSquares> data {};

    for (const auto pos : magic_enum::enum_values<BoardPosition>()) {
        uint8_t passedPos = pos - 8;
        const uint8_t file = passedPos % 8;
        uint64_t mask {};

        while (passedPos <= H8) {
            if (file < 7)
                mask |= 1ULL << (passedPos + 1);

            if (file > 0)
                mask |= 1ULL << (passedPos - 1);

            mask |= 1ULL << passedPos;

            passedPos -= 8;
        }

        data[pos] = mask;
    }

    return data;
}

}

constexpr auto s_rowMaskTable = generateRowMaskTable();
constexpr auto s_fileMaskTable = generateFileMaskTable();
constexpr auto s_isolationMaskTable = generateIsolationMaskTable();
constexpr auto s_whitePassedPawnMaskTable = generateWhitePassedPawnMaskTable();
constexpr auto s_blackPassedPawnMaskTable = generateBlackPassedPawnMaskTable();

constexpr auto s_whitePawnTable = std::to_array<int32_t>(
    { 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, -10, -10, 0, 0, 0,
        0, 0, 0, 5, 5, 0, 0, 0,
        5, 5, 10, 20, 20, 5, 5, 5,
        10, 10, 10, 20, 20, 10, 10, 10,
        20, 20, 20, 30, 30, 30, 20, 20,
        30, 30, 30, 40, 40, 30, 30, 30,
        90, 90, 90, 90, 90, 90, 90, 90 });

constexpr auto s_whiteKnightTable = std::to_array<int32_t>(
    { -5, -10, 0, 0, 0, 0, -10, -5,
        -5, 0, 0, 0, 0, 0, 0, -5,
        -5, 5, 20, 10, 10, 20, 5, -5,
        -5, 10, 20, 30, 30, 20, 10, -5,
        -5, 10, 20, 30, 30, 20, 10, -5,
        -5, 5, 20, 20, 20, 20, 5, -5,
        -5, 0, 0, 10, 10, 0, 0, -5,
        -5, 0, 0, 0, 0, 0, 0, -5 });

constexpr auto s_whiteBishopTable = std::to_array<int32_t>(
    { 0, 0, -10, 0, 0, -10, 0, 0,
        0, 30, 0, 0, 0, 0, 30, 0,
        0, 10, 0, 0, 0, 0, 10, 0,
        0, 0, 10, 20, 20, 10, 0, 0,
        0, 0, 10, 20, 20, 10, 0, 0,
        0, 0, 0, 10, 10, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0 });

constexpr auto s_whiteRookTable = std::to_array<int32_t>(
    { 0, 0, 0, 20, 20, 0, 0, 0,
        0, 0, 10, 20, 20, 10, 0, 0,
        0, 0, 10, 20, 20, 10, 0, 0,
        0, 0, 10, 20, 20, 10, 0, 0,
        0, 0, 10, 20, 20, 10, 0, 0,
        0, 0, 10, 20, 20, 10, 0, 0,
        50, 50, 50, 50, 50, 50, 50, 50,
        50, 50, 50, 50, 50, 50, 50, 50 });

constexpr auto s_whiteQueenTable = std::to_array<int32_t>(
    { -20, -10, -10, -5, -5, -10, -10, -20,
        -10, 0, 5, 0, 0, 0, 0, -10,
        -10, 5, 5, 5, 5, 5, 0, -10,
        0, 0, 5, 5, 5, 5, 0, -5,
        -5, 0, 5, 5, 5, 5, 0, -5,
        -10, 0, 5, 5, 5, 5, 0, -10,
        -10, 0, 0, 0, 0, 0, 0, -10,
        -20, -10, -10, -5, -5, -10, -10, -20 });

constexpr auto s_whiteKingTable = std::to_array<int32_t>(
    { 0, 0, 5, 0, -15, 0, 10, 0,
        0, 5, 5, -5, -5, 0, 5, 0,
        0, 0, 5, 10, 10, 5, 0, 0,
        0, 5, 10, 20, 20, 10, 5, 0,
        0, 5, 10, 20, 20, 10, 5, 0,
        0, 5, 5, 10, 10, 5, 5, 0,
        0, 0, 5, 5, 5, 5, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0 });

constexpr auto s_blackPawnTable = std::to_array<int32_t>(
    { 90, 90, 90, 90, 90, 90, 90, 90,
        30, 30, 30, 40, 40, 30, 30, 30,
        20, 20, 20, 30, 30, 30, 20, 20,
        10, 10, 10, 20, 20, 10, 10, 10,
        5, 5, 10, 20, 20, 5, 5, 5,
        0, 0, 0, 5, 5, 0, 0, 0,
        0, 0, 0, -10, -10, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0 });

constexpr auto s_blackKnightTable = std::to_array<int32_t>(
    { -5, 0, 0, 0, 0, 0, 0, -5,
        -5, 0, 0, 10, 10, 0, 0, -5,
        -5, 5, 20, 20, 20, 20, 5, -5,
        -5, 10, 20, 30, 30, 20, 10, -5,
        -5, 10, 20, 30, 30, 20, 10, -5,
        -5, 5, 20, 10, 10, 20, 5, -5,
        -5, 0, 0, 0, 0, 0, 0, -5,
        -5, -10, 0, 0, 0, 0, -10, -5 });

constexpr auto s_blackBishopTable = std::to_array<int32_t>(
    { 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 10, 10, 0, 0, 0,
        0, 0, 10, 20, 20, 10, 0, 0,
        0, 0, 10, 20, 20, 10, 0, 0,
        0, 10, 0, 0, 0, 0, 10, 0,
        0, 30, 0, 0, 0, 0, 30, 0,
        0, 0, -10, 0, 0, -10, 0, 0 });

constexpr auto s_blackRookTable = std::to_array<int32_t>(
    { 50, 50, 50, 50, 50, 50, 50, 50,
        50, 50, 50, 50, 50, 50, 50, 50,
        0, 0, 10, 20, 20, 10, 0, 0,
        0, 0, 10, 20, 20, 10, 0, 0,
        0, 0, 10, 20, 20, 10, 0, 0,
        0, 0, 10, 20, 20, 10, 0, 0,
        0, 0, 10, 20, 20, 10, 0, 0,
        0, 0, 0, 20, 20, 0, 0, 0 });

constexpr auto s_blackQueenTable = std::to_array<int32_t>(
    { -20, -10, -10, -5, -5, -10, -10, -20,
        -10, 0, 0, 0, 0, 0, 0, -10,
        -10, 0, 5, 5, 5, 5, 0, -10,
        -5, 0, 5, 5, 5, 5, 0, -5,
        0, 0, 5, 5, 5, 5, 0, -5,
        -10, 5, 5, 5, 5, 5, 0, -10,
        -10, 0, 5, 0, 0, 0, 0, -10,
        -20, -10, -10, -5, -5, -10, -10, -20 });

constexpr auto s_blackKingTable = std::to_array<int32_t>(
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
