#pragma once

#include "board_defs.h"

#include "src/movement/bishops.h"
#include "src/movement/kings.h"
#include "src/movement/knights.h"
#include "src/movement/rooks.h"

#include <bit>
#include <cstdint>

namespace gen {

namespace {

// most valuable victim & less valuable attacker

/*
    (Victims) Pawn Knight Bishop   Rook  Queen   King
  (Attackers)
        Pawn   105    205    305    405    505    605
      Knight   104    204    304    404    504    604
      Bishop   103    203    303    403    503    603
        Rook   102    202    302    402    502    602
       Queen   101    201    301    401    501    601
        King   100    200    300    400    500    600
*/

static int s_mvvLvaTable[6][6] = {
    { 105, 205, 305, 405, 505, 605 },
    { 104, 204, 304, 404, 504, 604 },
    { 103, 203, 303, 403, 503, 603 },
    { 102, 202, 302, 402, 502, 602 },
    { 101, 201, 301, 401, 501, 601 },
    { 100, 200, 300, 400, 500, 600 },
};

}

constexpr static inline int16_t getMvvLvaScore(Piece attacker, Piece victim)
{
    return s_mvvLvaTable[static_cast<int>(attacker)][static_cast<int>(victim)];
}

constexpr static inline uint64_t getKnightAttacks(uint64_t knights)
{
    uint64_t attacks = {};
    while (knights) {
        const int from = std::countr_zero(knights);
        knights &= knights - 1;

        attacks |= movement::s_knightsTable.at(from);
    }

    return attacks;
}

constexpr static inline uint64_t getRookAttacks(uint64_t rooks, uint64_t occupancy)
{
    uint64_t attacks = {};
    while (rooks) {
        const int from = std::countr_zero(rooks);
        rooks &= rooks - 1;

        attacks |= movement::getRookAttacks(from, occupancy);
    }

    return attacks;
}

constexpr static inline uint64_t getBishopAttacks(uint64_t bishops, uint64_t occupancy)
{
    uint64_t attacks = {};
    while (bishops) {
        const int from = std::countr_zero(bishops);
        bishops &= bishops - 1;

        attacks |= movement::getBishopAttacks(from, occupancy);
    }

    return attacks;
}

constexpr static inline uint64_t getQueenAttacks(uint64_t queens, uint64_t occupancy)
{
    uint64_t attacks = {};
    while (queens) {
        const int from = std::countr_zero(queens);
        queens &= queens - 1;

        attacks |= movement::getRookAttacks(from, occupancy);
        attacks |= movement::getBishopAttacks(from, occupancy);
    }

    return attacks;
}

constexpr static inline uint64_t getKingAttacks(uint64_t king)
{
    if (king == 0) {
        return 0;
    }

    const int from = std::countr_zero(king);
    return movement::s_kingsTable.at(from);
}

constexpr static inline uint64_t getWhitePawnAttacks(uint64_t pawns)
{
    uint64_t attacks = ((pawns & ~s_aFileMask) << 7);
    attacks |= ((pawns & ~s_hFileMask) << 9);

    return attacks;
}

constexpr static inline uint64_t getBlackPawnAttacks(uint64_t pawns)
{
    uint64_t attacks = ((pawns & ~s_aFileMask) >> 7);
    attacks |= ((pawns & ~s_hFileMask) >> 9);

    return attacks;
}

}
