#pragma once

#include "magic_enum/magic_enum.hpp"
#include "src/board_defs.h"

#include <cstdint>
#include <span>

namespace movement {

enum MoveFlags : uint8_t {
    None = 0,
    Capture = 1 << 0,
    Castle = 1 << 1,
    EnPassant = 1 << 2,
};

struct Move {
    uint8_t from {};
    uint8_t to {};
    MoveFlags flags {};

    friend bool operator<=>(const Move&, const Move&) = default;

    std::string toString() const
    {
        std::string result;
        result.resize(4); // Preallocate space

        result[0] = 'a' + (from % 8); // Column
        result[1] = '1' + (from / 8); // Row
        result[2] = 'a' + (to % 8); // Column
        result[3] = '1' + (to / 8); // Row

        return result;
    }
};

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

}

// Has to be in neutral namespace
template<>
struct magic_enum::customize::enum_range<movement::MoveFlags> {
    static constexpr bool is_flags = true;
};
