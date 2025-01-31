
#pragma once

#include "board_defs.h"
#include "knights_magic.h"

#include <bit>
#include <cstdint>

namespace gen {

constexpr static inline uint64_t getKnightAttacks(uint64_t knights)
{
    uint64_t attacks = {};
    while (knights) {
        const int from = std::countr_zero(knights);
        knights &= knights - 1;

        attacks |= magic::s_knightsTable.at(from);
    }

    return attacks;
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
