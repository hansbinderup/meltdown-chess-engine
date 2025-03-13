#pragma once

#include <cstdint>
#include <immintrin.h>

namespace magics {
namespace pext {

// Efficient dense indexing of blockable pieces
// https://www.chessprogramming.org/BMI2#PEXT_Bitboards
constexpr inline int magicIndex(uint64_t occupancy, uint64_t mask, bool is_compile_time = false)
{
    // pext_u64 is not available at compile time
    if (is_compile_time) {
        uint64_t res = 0, bit = 1;
        for (uint64_t m = mask; m; m &= (m - 1)) {
            if (occupancy & (m & -m)) {
                res |= bit;
            }
            bit <<= 1;
        }
        return res;
    } else {
        return _pext_u64(occupancy, mask);
    }
}

} // end namespace pext
} // end namespace magics
