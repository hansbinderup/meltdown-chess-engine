#pragma once

#include "src/movement/move_types.h"

#include <cstdint>

namespace movement {

namespace {

constexpr static inline void backtrackPawnMoves(ValidMoves& validMoves, uint64_t moves, int8_t bitCnt, MoveFlags flags = MoveFlags::None)
{
    while (moves) {
        int to = std::countr_zero(moves); // Find first set bit
        moves &= moves - 1; // Clear bit
        int from = to - bitCnt; // Backtrack the pawn's original position
        validMoves.addMove({ static_cast<uint8_t>(from), static_cast<uint8_t>(to), flags });
    }
}

}

constexpr static inline void getWhitePawnMoves(ValidMoves& validMoves, uint64_t pawns, uint64_t whiteOccupation, uint64_t blackOccupation)
{
    const uint64_t allOccupation = whiteOccupation | blackOccupation;

    /*
     * straight -> full row bit shift where there no other pieces
     * straightDouble -> same as straight but only second row BUT also where third row is not occupied - to compensate for this,
     * we move the black pieces a row back to block these out on a binary level
     * attackLeft -> if straight is 8 then left must be 7 (all shifted one to the side). This should never happen to a file pawns.
     * attackRight -> same as left but the other direction
     */
    uint64_t moveStraight = ((pawns & ~s_row7Mask) << 8) & ~allOccupation;
    uint64_t moveStraightDouble = ((pawns & s_row2Mask) << 16) & ~(allOccupation | (allOccupation << 8));
    uint64_t attackLeft = ((pawns & ~s_row7Mask & ~s_aFileMask) << 7) & blackOccupation;
    uint64_t attackRight = ((pawns & ~s_row7Mask & ~s_hFileMask) << 9) & blackOccupation;

    backtrackPawnMoves(validMoves, moveStraight, 8);
    backtrackPawnMoves(validMoves, moveStraightDouble, 16);
    backtrackPawnMoves(validMoves, attackLeft, 7, MoveFlags::Capture);
    backtrackPawnMoves(validMoves, attackRight, 9, MoveFlags::Capture);
}

constexpr static inline void getBlackPawnMoves(ValidMoves& validMoves, uint64_t pawns, uint64_t whiteOccupation, uint64_t blackOccupation)
{
    const uint64_t allOccupation = whiteOccupation | blackOccupation;

    uint64_t moveStraight = ((pawns & ~s_row2Mask) >> 8) & ~allOccupation;
    uint64_t moveStraightDouble = ((pawns & s_row7Mask) >> 16) & ~(allOccupation | (allOccupation >> 8));
    uint64_t attackLeft = ((pawns & ~s_row2Mask & ~s_aFileMask) >> 9) & whiteOccupation;
    uint64_t attackRight = ((pawns & ~s_row2Mask & ~s_hFileMask) >> 7) & whiteOccupation;

    backtrackPawnMoves(validMoves, moveStraight, -8);
    backtrackPawnMoves(validMoves, moveStraightDouble, -16);
    backtrackPawnMoves(validMoves, attackLeft, -9, MoveFlags::Capture);
    backtrackPawnMoves(validMoves, attackRight, -7, MoveFlags::Capture);
}

}

