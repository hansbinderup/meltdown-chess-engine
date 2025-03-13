#pragma once

#include "board_defs.h"
#include "magics/magics.h"

#include <array>
#include <cstdint>

#include "magics/hashing.h"
#include "magics/pext.h"

namespace movegen {

namespace {

constexpr uint64_t rookMask(int square)
{
    uint64_t attacks = 0;

    const int tr = square / 8;
    const int tf = square % 8;

    for (int r = tr + 1; r <= 6; ++r)
        attacks |= (1ULL << (r * 8 + tf));
    for (int r = tr - 1; r >= 1; --r)
        attacks |= (1ULL << (r * 8 + tf));
    for (int f = tf + 1; f <= 6; ++f)
        attacks |= (1ULL << (tr * 8 + f));
    for (int f = tf - 1; f >= 1; --f)
        attacks |= (1ULL << (tr * 8 + f));

    return attacks;
}

// Generate a table with all rook moves
constexpr std::array<uint64_t, 64> generateRookMasksTable()
{
    std::array<uint64_t, 64> table = {};
    for (uint8_t i = 0; i < 64; ++i) {
        table[i] = rookMask(i);
    }
    return table;
}

constexpr auto s_rookMasksTable = generateRookMasksTable();

constexpr uint64_t rookAttacksWithBlock(int square, uint64_t block)
{
    uint64_t attacks = 0;

    const int tr = square / 8;
    const int tf = square % 8;

    // Generate rook attacks
    for (int r = tr + 1; r <= 7; ++r) {
        uint64_t pos = 1ULL << (r * 8 + tf);
        attacks |= pos;
        if (pos & block)
            break;
    }

    for (int r = tr - 1; r >= 0; --r) {
        uint64_t pos = 1ULL << (r * 8 + tf);
        attacks |= pos;
        if (pos & block)
            break;
    }

    for (int f = tf + 1; f <= 7; ++f) {
        uint64_t pos = 1ULL << (tr * 8 + f);
        attacks |= pos;
        if (pos & block)
            break;
    }

    for (int f = tf - 1; f >= 0; --f) {
        uint64_t pos = 1ULL << (tr * 8 + f);
        attacks |= pos;
        if (pos & block)
            break;
    }

    return attacks;
}

constexpr auto generateRookAttackTablePext()
{
    std::array<magic::SliderAttackTable, 64> attacks;

    for (int square = 0; square < 64; square++) {

        const uint64_t attackMask = s_rookMasksTable[square];
        const int relevantBitsCount = std::popcount(attackMask);
        const int occupancyIndicies = (1 << relevantBitsCount);

        for (int index = 0; index < occupancyIndicies; index++) {
            const uint64_t occupancy = magic::set_occupancy(index, relevantBitsCount, attackMask);
            const int magicIndex = magics::pext::magicIndex(occupancy, attackMask, true);
            attacks[square][magicIndex] = rookAttacksWithBlock(square, occupancy);
        }
    }

    return attacks;
}

constexpr std::array<magic::SliderAttackTable, 64> generateRookAttackTableHashing()
{
    std::array<magic::SliderAttackTable, 64> attacks;

    for (int square = 0; square < 64; square++) {

        const uint64_t attackMask = s_rookMasksTable[square];
        const int relevantBitsCount = std::popcount(attackMask);
        const int occupancyIndicies = (1 << relevantBitsCount);

        for (int index = 0; index < occupancyIndicies; index++) {
            const uint64_t occupancy = magic::set_occupancy(index, relevantBitsCount, attackMask);
            // TODO here, magicIndex is applying the attack mask after already applied.
            // Happens at compile time, do we care?
            const int magicIndex = magics::hashing::magicIndex(occupancy, attackMask, square);
            attacks[square][magicIndex] = rookAttacksWithBlock(square, occupancy);
        }
    }

    return attacks;
}

constexpr std::array<magic::SliderAttackTable, 64> generateRookAttackTable()
{

#ifdef USE_BMI2
    return generateRookAttackTablePext();
#else
    return generateRookAttackTableHashing();
#endif
}

constexpr auto s_rookAttackTable = generateRookAttackTable();
} // end namespace

// https://www.chessprogramming.org/BMI2#PEXT_Bitboards
static inline uint64_t getRookMovesPext(BoardPosition pos, uint64_t occupancy)
{
    occupancy = magics::pext::magicIndex(occupancy, s_rookMasksTable[pos]);

    return s_rookAttackTable[pos][occupancy];
}

/* https://www.chessprogramming.org/Magic_Bitboards */
static inline uint64_t getRookMovesHashing(BoardPosition pos, uint64_t occupancy)
{
    occupancy = magics::hashing::magicIndex(occupancy, s_rookMasksTable[pos], pos);

    return s_rookAttackTable[pos][occupancy];
}

static inline uint64_t getRookMoves(BoardPosition pos, uint64_t occupancy)
{
#ifdef USE_BMI2
    return getRookMovesPext(pos, occupancy);
#else
    return getRookMovesHashing(pos, occupancy);
#endif
}

} // end namespace movegen
