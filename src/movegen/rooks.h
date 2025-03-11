#pragma once

#include "magics/magics.h"

#include <array>
#include <cstdint>

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

constexpr auto generateRookAttackTable()
{
    std::array<magic::SliderAttackTable, 64> attacks;

    for (int square = 0; square < 64; square++) {

        const uint64_t attackMask = s_rookMasksTable[square];
        const int relevantBitsCount = std::popcount(attackMask);
        const int occupancyIndicies = (1 << relevantBitsCount);

        for (int index = 0; index < occupancyIndicies; index++) {
            const uint64_t occupancy = magic::set_occupancy(index, relevantBitsCount, attackMask);
            const int magicIndex = (occupancy * magic::s_rooksMagic[square]) >> (64 - magic::s_rookRelevantBits[square]);

            attacks[square][magicIndex] = rookAttacksWithBlock(square, occupancy);
        }
    }

    return attacks;
}

constexpr auto s_rookAttackTable = generateRookAttackTable();

}

static inline uint64_t getRookMoves(int square, uint64_t occupancy)
{
    // https://www.chessprogramming.org/Magic_Bitboards
    occupancy &= s_rookMasksTable[square];
    occupancy *= magic::s_rooksMagic[square];
    occupancy >>= 64 - magic::s_rookRelevantBits[square];

    return s_rookAttackTable[square][occupancy];
}

}
