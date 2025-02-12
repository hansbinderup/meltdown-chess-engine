#pragma once

#include "magic_enum/magic_enum.hpp"
#include "magic_enum/magic_enum_flags.hpp"
#include "src/board_defs.h"

#include <cstdint>
#include <span>

/* required for enum flags to work */
using namespace magic_enum::bitwise_operators;

namespace movement {

enum class MoveFlags : std::uint8_t {
    None = 0,
    Capture = 1 << 0,
    Castle = 1 << 1,
    EnPassant = 1 << 2,
    PromoteKnight = 1 << 3,
    PromoteBishop = 1 << 4,
    PromoteRook = 1 << 5,
    PromoteQueen = 1 << 6,
};

constexpr static inline auto s_moveFlagPromoteMask = MoveFlags::PromoteKnight | MoveFlags::PromoteBishop | MoveFlags::PromoteRook | MoveFlags::PromoteQueen;

struct Move {
    uint8_t from {};
    uint8_t to {};
    MoveFlags flags { MoveFlags::None };

    friend bool operator<=>(const Move&, const Move&) = default;

    constexpr inline uint64_t fromSquare() const
    {
        return 1ULL << from;
    }

    constexpr inline uint64_t toSquare() const
    {
        return 1ULL << to;
    }

    constexpr inline std::string toString() const
    {
        std::string result;
        result.resize(5); // Preallocate space

        result[0] = 'a' + (from % 8); // Column
        result[1] = '1' + (from / 8); // Row
        result[2] = 'a' + (to % 8); // Column
        result[3] = '1' + (to / 8); // Row

        if (magic_enum::enum_flags_test_any(flags, s_moveFlagPromoteMask)) {
            if (magic_enum::enum_flags_test_any(flags, MoveFlags::PromoteKnight))
                result[4] = 'n';
            else if (magic_enum::enum_flags_test_any(flags, MoveFlags::PromoteBishop))
                result[4] = 'b';
            else if (magic_enum::enum_flags_test_any(flags, MoveFlags::PromoteRook))
                result[4] = 'r';
            else if (magic_enum::enum_flags_test_any(flags, MoveFlags::PromoteQueen))
                result[4] = 'q';
        } else {
            result.resize(4);
        }

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
