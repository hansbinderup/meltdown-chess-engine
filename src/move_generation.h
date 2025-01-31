#pragma once

#include "board_defs.h"
#include "kings_magic.h"
#include "knights_magic.h"

#include <array>
#include <span>

namespace gen {

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

namespace {

constexpr static inline void backtrackPawnMoves(ValidMoves& validMoves, uint64_t moves, int8_t bitCnt)
{
    while (moves) {
        int to = std::countr_zero(moves); // Find first set bit
        moves &= moves - 1; // Clear bit
        int from = to - bitCnt; // Backtrack the pawn's original position
        validMoves.addMove({ static_cast<uint8_t>(from), static_cast<uint8_t>(to) });
    }
}

}

constexpr static inline void getWhitePawnMoves(gen::ValidMoves& validMoves, uint64_t pawns, uint64_t whiteOccupation, uint64_t blackOccupation)
{
    const uint64_t allOccupation = whiteOccupation | blackOccupation;

    uint64_t moveStraight = ((pawns & ~s_row7Mask) << 8) & ~allOccupation;
    uint64_t moveStraightDouble = ((pawns & s_row2Mask) << 16) & ~allOccupation;
    uint64_t attackLeft = ((pawns & ~s_row7Mask & ~s_aFileMask) << 7) & blackOccupation;
    uint64_t attackRight = ((pawns & ~s_row7Mask & ~s_hFileMask) << 9) & blackOccupation;

    backtrackPawnMoves(validMoves, moveStraight, 8);
    backtrackPawnMoves(validMoves, moveStraightDouble, 16);
    backtrackPawnMoves(validMoves, attackLeft, 7);
    backtrackPawnMoves(validMoves, attackRight, 9);
}

constexpr static inline void getBlackPawnMoves(gen::ValidMoves& validMoves, uint64_t pawns, uint64_t whiteOccupation, uint64_t blackOccupation)
{
    const uint64_t allOccupation = whiteOccupation | blackOccupation;

    uint64_t moveStraight = ((pawns & ~s_row2Mask) >> 8) & ~allOccupation;
    uint64_t moveStraightDouble = ((pawns & s_row7Mask) >> 16) & ~allOccupation;
    uint64_t attackLeft = ((pawns & ~s_row2Mask & ~s_aFileMask) >> 7) & whiteOccupation;
    uint64_t attackRight = ((pawns & ~s_row2Mask & ~s_hFileMask) >> 9) & whiteOccupation;

    backtrackPawnMoves(validMoves, moveStraight, -8);
    backtrackPawnMoves(validMoves, moveStraightDouble, -16);
    backtrackPawnMoves(validMoves, attackLeft, -7);
    backtrackPawnMoves(validMoves, attackRight, -9);
}

constexpr static inline void getKnightMoves(ValidMoves& validMoves, uint64_t knights, uint64_t ownOccupation)
{
    while (knights) {
        const int from = std::countr_zero(knights);
        knights &= knights - 1;

        uint64_t moves = magic::s_knightsTable.at(from) & ~ownOccupation;

        while (moves) {
            int to = std::countr_zero(moves);
            moves &= moves - 1;
            validMoves.addMove({ static_cast<uint8_t>(from), static_cast<uint8_t>(to) });
        }
    }
}

constexpr static inline void getKingMoves(ValidMoves& validMoves, uint64_t king, uint64_t ownOccupation, uint64_t attacks)
{
    const int from = std::countr_zero(king);
    king &= king - 1;

    uint64_t moves = magic::s_kingsTable.at(from) & ~ownOccupation & ~attacks;

    while (moves) {
        int to = std::countr_zero(moves);
        moves &= moves - 1;
        validMoves.addMove({ static_cast<uint8_t>(from), static_cast<uint8_t>(to) });
    }
}

}
