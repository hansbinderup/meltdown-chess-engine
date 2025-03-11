#pragma once

#include "board_defs.h"
#include <bit>
#include <cstdint>

namespace helper {

[[nodiscard]] constexpr uint64_t positionToSquare(BoardPosition pos) noexcept
{
    return 1ULL << pos;
}

[[nodiscard]] constexpr BoardPosition lsbToPosition(uint64_t piece) noexcept
{
    return static_cast<BoardPosition>(std::countr_zero(piece));
}

template<typename Func>
constexpr void bitIterate(uint64_t data, Func&& fnc) noexcept
    requires std::is_invocable_v<Func, BoardPosition>
{
    while (data) {
        fnc(lsbToPosition(data));
        data &= data - 1;
    }
}

}

