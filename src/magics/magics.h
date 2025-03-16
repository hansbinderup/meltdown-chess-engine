#pragma once

#include <array>
#include <cstdint>

namespace magic {

using SliderAttackTable = std::array<uint64_t, 4096>;

constexpr bool getBit(uint64_t bitboard, int square)
{
    return (bitboard & (1ULL << square)) != 0;
}

constexpr void popBit(uint64_t& bitboard, int square)
{
    if (getBit(bitboard, square)) {
        bitboard ^= (1ULL << square);
    }
}

constexpr uint64_t set_occupancy(int index, int bitsInMask, uint64_t attackMask)
{
    uint64_t occupancy = 0;

    for (int count = 0; count < bitsInMask; ++count) {
        if (attackMask == 0)
            break; // Prevents undefined behavior

        // Get LS1B index
        int square = std::countr_zero(attackMask);

        // Clear LS1B in attack mask
        attackMask &= attackMask - 1;

        // Populate occupancy map if bit in index is set
        if (index & (1 << count)) {
            occupancy |= (1ULL << square);
        }
    }

    return occupancy;
}

} // end namespace magic
