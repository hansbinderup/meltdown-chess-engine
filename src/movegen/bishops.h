#pragma once

#include "magics/magics.h"
#include <array>
#include <cstdint>

namespace movegen {

namespace {

constexpr uint64_t bishopMask(int square)
{
    // result attacks bitboard
    uint64_t attacks = 0ULL;

    // init ranks & files
    int r, f;

    // init target rank & files
    int tr = square / 8;
    int tf = square % 8;

    // mask relevant bishop occupancy bits
    for (r = tr + 1, f = tf + 1; r <= 6 && f <= 6; r++, f++)
        attacks |= (1ULL << (r * 8 + f));
    for (r = tr - 1, f = tf + 1; r >= 1 && f <= 6; r--, f++)
        attacks |= (1ULL << (r * 8 + f));
    for (r = tr + 1, f = tf - 1; r <= 6 && f >= 1; r++, f--)
        attacks |= (1ULL << (r * 8 + f));
    for (r = tr - 1, f = tf - 1; r >= 1 && f >= 1; r--, f--)
        attacks |= (1ULL << (r * 8 + f));

    // return attack map
    return attacks;
}

// Generate a table with all bishop moves
constexpr std::array<uint64_t, 64> generateBishopMasksTable()
{
    std::array<uint64_t, 64> table = {};
    for (uint8_t i = 0; i < 64; ++i) {
        table[i] = bishopMask(i);
    }
    return table;
}

constexpr auto s_bishopMasksTable = generateBishopMasksTable();

constexpr uint64_t bishopAttacksWithBlock(int square, uint64_t block)
{
    // result attacks bitboard
    uint64_t attacks = 0ULL;

    // init ranks & files
    int r, f;

    // init target rank & files
    int tr = square / 8;
    int tf = square % 8;

    // generate bishop atacks
    for (r = tr + 1, f = tf + 1; r <= 7 && f <= 7; r++, f++) {
        attacks |= (1ULL << (r * 8 + f));
        if ((1ULL << (r * 8 + f)) & block)
            break;
    }

    for (r = tr - 1, f = tf + 1; r >= 0 && f <= 7; r--, f++) {
        attacks |= (1ULL << (r * 8 + f));
        if ((1ULL << (r * 8 + f)) & block)
            break;
    }

    for (r = tr + 1, f = tf - 1; r <= 7 && f >= 0; r++, f--) {
        attacks |= (1ULL << (r * 8 + f));
        if ((1ULL << (r * 8 + f)) & block)
            break;
    }

    for (r = tr - 1, f = tf - 1; r >= 0 && f >= 0; r--, f--) {
        attacks |= (1ULL << (r * 8 + f));
        if ((1ULL << (r * 8 + f)) & block)
            break;
    }

    // return attack map
    return attacks;
}

constexpr auto generateBishopAttackTable()
{
    std::array<magic::SliderAttackTable, 64> attacks;

    for (int square = 0; square < 64; square++) {
        const uint64_t attackMask = s_bishopMasksTable[square];
        const int relevantBitsCount = std::popcount(attackMask);
        const int occupancyIndicies = (1 << relevantBitsCount);

        for (int index = 0; index < occupancyIndicies; index++) {
            const uint64_t occupancy = magic::set_occupancy(index, relevantBitsCount, attackMask);
            const int magicIndex = (occupancy * magic::s_bishopsMagic[square]) >> (64 - magic::s_bishopRelevantBits[square]);

            attacks[square][magicIndex] = bishopAttacksWithBlock(square, occupancy);
        }
    }

    return attacks;
}

constexpr auto s_bishopAttackTable = generateBishopAttackTable();

}

static inline uint64_t getBishopMoves(int square, uint64_t occupancy)
{
    occupancy &= s_bishopMasksTable[square];
    occupancy *= magic::s_bishopsMagic[square];
    occupancy >>= 64 - magic::s_bishopRelevantBits[square];

    return s_bishopAttackTable[square][occupancy];
}

}

