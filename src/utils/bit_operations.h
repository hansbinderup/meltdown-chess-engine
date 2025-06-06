#pragma once

#include "core/board_defs.h"
#include <bit>
#include <cstdint>

namespace utils {

[[nodiscard]] constexpr uint64_t positionToSquare(BoardPosition pos) noexcept
{
    return 1ULL << pos;
}

[[nodiscard]] constexpr uint8_t verticalDistance(BoardPosition from, BoardPosition to) noexcept
{
    const auto fromRow = from / 8;
    const auto toRow = to / 8;

    return std::abs(fromRow - toRow);
}

template<Player player>
constexpr uint8_t relativeRow(BoardPosition pos)
{
    if constexpr (player == PlayerWhite) {
        return pos / 8;
    } else {
        return 7 - (pos / 8);
    }
}

[[nodiscard]] constexpr BoardPosition lsbToPosition(uint64_t piece) noexcept
{
    return static_cast<BoardPosition>(std::countr_zero(piece));
}

/* iterates over each set bit in the bitboard `data` and calls `fnc` with the corresponding BoardPosition
 * `fnc` is invoked once per set bit, with the position of the least significant bit */
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

