#pragma once

#include "board_defs.h"
#include <array>
#include <span>

namespace gen {

struct Move {
    uint8_t from {};
    uint8_t to {};
};

struct ValidMoves {
    std::span<const Move> getMoves() const
    {
        return std::span(moves.data(), count);
    }

    void addMove(Move move)
    {
        moves[count++] = std::move(move);
    }

private:
    std::array<Move, s_maxMoves> moves {};
    uint16_t count = 0;
};

constexpr static inline void getWhitePawnMoves(gen::ValidMoves& validMoves, uint64_t pawns, uint64_t whiteOccupation, uint64_t blackOccupation)
{
    const uint64_t allOccupation = whiteOccupation | blackOccupation;

    uint64_t moveStraight = ((pawns & ~s_row7Mask) << 8) & ~allOccupation;
    uint64_t moveStraightDouble = ((pawns & s_row2Mask) << 16) & ~allOccupation;
    uint64_t attackLeft = ((pawns & ~s_row7Mask & ~s_aFileMask) << 7) & blackOccupation;
    uint64_t attackRight = ((pawns & ~s_row7Mask & ~s_hFileMask) << 9) & blackOccupation;

    while (moveStraight) {
        int to = std::countr_zero(moveStraight); // Find first set bit
        moveStraight &= moveStraight - 1; // Clear bit
        int from = to - 8; // Backtrack the pawn's original position
        validMoves.addMove({ static_cast<uint8_t>(from), static_cast<uint8_t>(to) });
    }

    while (moveStraightDouble) {
        int to = std::countr_zero(moveStraightDouble);
        moveStraightDouble &= moveStraightDouble - 1;
        int from = to - 16;
        validMoves.addMove({ static_cast<uint8_t>(from), static_cast<uint8_t>(to) });
    }

    while (attackLeft) {
        int to = std::countr_zero(attackLeft);
        attackLeft &= attackLeft - 1;
        int from = to - 7;
        validMoves.addMove({ static_cast<uint8_t>(from), static_cast<uint8_t>(to) });
    }

    while (attackRight) {
        int to = std::countr_zero(attackRight);
        attackRight &= attackRight - 1;
        int from = to - 9;
        validMoves.addMove({ static_cast<uint8_t>(from), static_cast<uint8_t>(to) });
    }
}

}

