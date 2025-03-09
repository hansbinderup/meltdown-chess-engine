#pragma once

#include "board_defs.h"
#include <bit>
#include <cstdint>

namespace helper {

template<typename Func>
constexpr static inline void bitIterate(uint64_t data, Func&& fnc)
    requires std::is_invocable_v<Func, BoardPosition>
{
    while (data) {
        int pos = std::countr_zero(data);
        fnc(static_cast<BoardPosition>(pos));
        data &= data - 1;
    }
}

}

