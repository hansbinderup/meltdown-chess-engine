#pragma once

#include "src/board_defs.h"

#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

namespace movement {

struct Move {
    uint8_t from {};
    uint8_t to {};

    friend bool operator<=>(const Move&, const Move&) = default;
};

static inline std::optional<Move> moveFromString(std::string_view sv)
{
    if (sv.size() < 4) {
        return std::nullopt;
    }

    // quick and dirty way to transform move string to integer values
    const uint8_t fromIndex = (sv.at(0) - 'a') + (sv.at(1) - '1') * 8;
    const uint8_t toIndex = (sv.at(2) - 'a') + (sv.at(3) - '1') * 8;

    return movement::Move { fromIndex, toIndex };
}

static inline std::string moveToString(Move move)
{
    std::string result;
    result.resize(4); // Preallocate space

    result[0] = 'a' + (move.from % 8); // Column
    result[1] = '1' + (move.from / 8); // Row
    result[2] = 'a' + (move.to % 8); // Column
    result[3] = '1' + (move.to / 8); // Row

    return result;
}

class ValidMoves {
public:
    std::span<const Move> getMoves() const
    {
        return std::span(m_moves.data(), m_count);
    }

    std::span<Move> getMoves()
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

