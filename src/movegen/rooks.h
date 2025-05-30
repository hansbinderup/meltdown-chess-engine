#pragma once

#include "core/board_defs.h"
#include "magics/magics.h"

#include <array>
#include <cstdint>

#ifdef __BMI2__
#include <immintrin.h>
#else
#include "magics/hashing.h"
#endif

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

#ifdef __BMI2__
            // pext_u64 is not available at compile time
            int magicIndex = 0, bit = 1;
            for (uint64_t m = attackMask; m; m &= (m - 1)) {
                if (occupancy & (m & -m)) {
                    magicIndex |= bit;
                }
                bit <<= 1;
            }
#else
            const int magicIndex = (occupancy * magic::hashing::rooks::s_magic[square]) >> (64 - magic::hashing::rooks::s_relevantBits[square]);
#endif
            attacks[square][magicIndex] = rookAttacksWithBlock(square, occupancy);
        }
    }

    return attacks;
}

static inline auto s_rookAttackTable = generateRookAttackTable();

} // end namespace

static inline uint64_t getRookMoves(BoardPosition pos, uint64_t occupancy)
{
#ifdef __BMI2__
    // https://www.chessprogramming.org/BMI2#PEXT_Bitboards
    occupancy = _pext_u64(occupancy, s_rookMasksTable[pos]);
#else
    /* https://www.chessprogramming.org/Magic_Bitboards */
    occupancy &= s_rookMasksTable[pos];
    occupancy *= magic::hashing::rooks::s_magic[pos];
    occupancy >>= 64 - magic::hashing::rooks::s_relevantBits[pos];
#endif
    return s_rookAttackTable[pos][occupancy];
}

} // end namespace movegen
