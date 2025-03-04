#pragma once

#include "src/board_defs.h"
#include <bit>
#include <cstdint>
#include <functional>

namespace helper {

constexpr static inline void bitIterate(uint64_t data, std::function<void(BoardPosition)> fnc)
{
    while (data) {
        const int pos = std::countr_zero(data); /* find the lowest set bit */
        data &= (data - 1); /* remove the lowest set bit */

        fnc(static_cast<BoardPosition>(pos));
    }
}

}

