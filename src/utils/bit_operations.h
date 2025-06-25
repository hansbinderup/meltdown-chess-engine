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

[[nodiscard]] constexpr uint8_t horizontalDistance(BoardPosition from, BoardPosition to) noexcept
{
    const auto fromRow = from % 8;
    const auto toRow = to % 8;

    return std::abs(fromRow - toRow);
}

/* Chebyshev Distance https://www.chessprogramming.org/Distance */
[[nodiscard]] constexpr uint8_t absoluteDistance(BoardPosition from, BoardPosition to) noexcept
{
    return std::max(verticalDistance(from, to), horizontalDistance(from, to));
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

/* helper to compute a mask with provided pieces pushed forward */
template<Player player>
static inline uint64_t pushForward(uint64_t pieces)
{
    if constexpr (player == PlayerWhite) {
        return pieces << 8;
    } else {
        return pieces >> 8;
    }
}

template<Player player>
static inline uint64_t pushForwardFromPos(BoardPosition pos)
{
    const uint64_t square = utils::positionToSquare(pos);
    return pushForward<player>(square);
}

[[nodiscard]] constexpr inline BoardPosition flipPosition(BoardPosition pos) noexcept
{
    return static_cast<BoardPosition>(pos ^ 56);
}

template<Player player>
constexpr inline BoardPosition relativePosition(BoardPosition pos) noexcept
{
    if constexpr (player == PlayerWhite) {
        return pos;
    } else {
        return flipPosition(pos);
    }
}

template<Player player>
constexpr inline bool isPawn(Piece piece)
{
    if constexpr (player == PlayerWhite) {
        return piece == WhitePawn;
    } else {
        return piece == BlackPawn;
    }
}

template<Player player>
constexpr inline bool isKing(Piece piece)
{
    if constexpr (player == PlayerWhite) {
        return piece == WhiteKing;
    } else {
        return piece == BlackKing;
    }
}

}
