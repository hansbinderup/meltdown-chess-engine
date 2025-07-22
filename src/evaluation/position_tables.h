#pragma once

#include "core/board_defs.h"
#include "magic_enum/magic_enum.hpp"
#include "utils/bit_operations.h"

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

/* Bitmask to find passed pawns
 * mask is the neighbor files + own file till the end of the board
 *
 * eg: table for F3 (White perspective):
 * -8- 0 0 0 0 1 1 1 0
 * -7- 0 0 0 0 1 1 1 0
 * -6- 0 0 0 0 1 1 1 0
 * -5- 0 0 0 0 1 1 1 0
 * -4- 0 0 0 0 1 1 1 0
 * -3- 0 0 0 0 0 0 0 0
 * -2- 0 0 0 0 0 0 0 0
 * -1- 0 0 0 0 0 0 0 0
 *     A B C D E F G H
 *
 * eg: table for C7 (Black perspective):
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
constexpr auto generatePassedPawnMaskTable()
{
    using PassedPawnMaskTable = std::array<uint64_t, s_amountSquares>;
    std::array<PassedPawnMaskTable, magic_enum::enum_count<Player>()> data {};

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

        data[PlayerWhite][pos] = mask;
    }

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

        data[PlayerBlack][pos] = mask;
    }

    return data;
}

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

constexpr auto generateOutpostSquareMaskTable()
{
    using OutpostMaskTable = std::array<uint64_t, s_amountSquares>;
    std::array<OutpostMaskTable, magic_enum::enum_count<Player>()> data {};

    constexpr auto passedPawnTable = generatePassedPawnMaskTable();
    constexpr auto fileMaskTable = generateFileMaskTable();

    for (const auto pos : magic_enum::enum_values<BoardPosition>()) {
        data[PlayerWhite][pos] = passedPawnTable[PlayerWhite][pos] & ~fileMaskTable[pos];
        data[PlayerBlack][pos] = passedPawnTable[PlayerBlack][pos] & ~fileMaskTable[pos];
    }
    return data;
}

constexpr auto generatePawnAttackMaskTable()
{
    using PawnAttackMaskTable = std::array<uint64_t, s_amountSquares>;
    std::array<PawnAttackMaskTable, magic_enum::enum_count<Player>()> data {};

    for (const auto pos : magic_enum::enum_values<BoardPosition>()) {
        const uint64_t square = utils::positionToSquare(pos);

        if ((square & s_aFileMask) == 0) {
            data[PlayerWhite][pos] |= square << 7;
            data[PlayerBlack][pos] |= square >> 9;
        }

        if ((square & s_hFileMask) == 0) {
            data[PlayerWhite][pos] |= square << 9;
            data[PlayerBlack][pos] |= square >> 7;
        }
    }

    return data;
}

constexpr auto generatePawnConnectedMaskTable()
{
    using PawnConnectedMaskTable = std::array<uint64_t, s_amountSquares>;
    std::array<PawnConnectedMaskTable, magic_enum::enum_count<Player>()> data {};

    const auto attacks = generatePawnAttackMaskTable();

    for (uint8_t pos = 8; pos < 56; pos++) {
        data[PlayerWhite][pos] = attacks[PlayerBlack][pos] | attacks[PlayerBlack][pos + 8];
        data[PlayerBlack][pos] = attacks[PlayerWhite][pos] | attacks[PlayerWhite][pos - 8];
    }
    return data;
}

constexpr auto generateAdjecentFileMask()
{
    std::array<uint64_t, 8> data {};

    constexpr uint64_t fileMasks[8] = {
        0x0101010101010101ull, /* file A */
        0x0202020202020202ull, /* file B */
        0x0404040404040404ull, /* file C */
        0x0808080808080808ull, /* file D */
        0x1010101010101010ull, /* file E */
        0x2020202020202020ull, /* file F */
        0x4040404040404040ull, /* file G */
        0x8080808080808080ull /* file H */
    };

    for (uint8_t file = 0; file < 8; ++file) {
        if (file > 0) {
            data[file] |= fileMasks[file - 1];
        }

        if (file < 7) {
            data[file] |= fileMasks[file + 1];
        }
    }

    return data;
}

}

constexpr auto s_rowMaskTable = generateRowMaskTable();
constexpr auto s_fileMaskTable = generateFileMaskTable();
constexpr auto s_isolationMaskTable = generateIsolationMaskTable();
constexpr auto s_passedPawnMaskTable = generatePassedPawnMaskTable();
constexpr auto s_outpostSquareMaskTable = generateOutpostSquareMaskTable();
constexpr auto s_pawnAttackMaskTable = generatePawnAttackMaskTable();
constexpr auto s_pawnConnectedMaskTable = generatePawnConnectedMaskTable();
constexpr auto s_adjecentFilesMask = generateAdjecentFileMask();

}
