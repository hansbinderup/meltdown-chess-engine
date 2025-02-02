#pragma once

#include "src/board_defs.h"

#include <cstdint>
#include <span>

namespace movement {

struct Move {
    uint8_t from {};
    uint8_t to {};
};

class ValidMoves {
public:
    std::span<const Move> getMoves() const
    {
        return std::span(m_moves.data(), m_count);
    }

    uint16_t count() const
    {
        return m_count;
    }

    void addMove(Move move)
    {
        m_moves[m_count++] = std::move(move);
    }

private:
    std::array<Move, s_maxMoves> m_moves {};
    uint16_t m_count = 0;
};

struct MoveDirection {
    const int8_t shiftDirection;
    const uint8_t shiftAmnt;

    constexpr static MoveDirection Up()
    {
        return { 1, 8 };
    }

    constexpr static MoveDirection Down()
    {
        return { -1, 8 };
    }

    constexpr static MoveDirection Right()
    {
        return { 1, 1 };
    }

    constexpr static MoveDirection Left()
    {
        return { -1, 1 };
    }

    constexpr static MoveDirection UpLeft()
    {
        return { 1, 7 };
    }

    constexpr static MoveDirection UpRight()
    {
        return { 1, 9 };
    }

    constexpr static MoveDirection DownLeft()
    {
        return { -1, 9 };
    }

    constexpr static MoveDirection DownRight()
    {
        return { -1, 7 };
    }
};

}

