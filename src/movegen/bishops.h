#pragma once

#include "board_defs.h"
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
            const int magicIndex = (occupancy * magic::hashing::bishops::s_magic[square]) >> (64 - magic::hashing::bishops::s_relevantBits[square]);
#endif
            attacks[square][magicIndex] = bishopAttacksWithBlock(square, occupancy);
        }
    }

    return attacks;
}

static inline auto s_bishopAttackTable = generateBishopAttackTable();

}

/* https://www.chessprogramming.org/Magic_Bitboards */
static inline uint64_t getBishopMoves(BoardPosition pos, uint64_t occupancy)
{
#ifdef __BMI2__
    // https://www.chessprogramming.org/BMI2#PEXT_Bitboards
    occupancy = _pext_u64(occupancy, s_bishopMasksTable[pos]);
#else
    /* https://www.chessprogramming.org/Magic_Bitboards */
    occupancy &= s_bishopMasksTable[pos];
    occupancy *= magic::hashing::bishops::s_magic[pos];
    occupancy >>= 64 - magic::hashing::bishops::s_relevantBits[pos];
#endif
    return s_bishopAttackTable[pos][occupancy];
}

}
