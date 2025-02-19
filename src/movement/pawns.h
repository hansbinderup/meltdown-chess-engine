#pragma once

#include "src/movement/move_types.h"

#include <cstdint>

namespace movement {

namespace {

constexpr static inline void backtrackPawnMoves(ValidMoves& validMoves, uint64_t moves, int8_t bitCnt, Piece piece, bool capture)
{
    while (moves) {
        int to = std::countr_zero(moves); // Find first set bit
        moves &= moves - 1; // Clear bit
        int from = to - bitCnt; // Backtrack the pawn's original position
        validMoves.addMove(movement::Move::create(static_cast<uint8_t>(from), static_cast<uint8_t>(to), piece, capture));
    }
}

constexpr static inline void backtrackPawnPromotions(ValidMoves& validMoves, uint64_t moves, int8_t bitCnt, Piece piece, bool capture)
{
    while (moves) {
        int to = std::countr_zero(moves); // Find first set bit
        moves &= moves - 1; // Clear bit
        int from = to - bitCnt; // Backtrack the pawn's original position

        /* add queen first - is usually the preferred piece */
        validMoves.addMove(movement::Move::createPromotion(static_cast<uint8_t>(from), static_cast<uint8_t>(to), piece, PromotionType::Queen, capture));
        validMoves.addMove(movement::Move::createPromotion(static_cast<uint8_t>(from), static_cast<uint8_t>(to), piece, PromotionType::Knight, capture));
        validMoves.addMove(movement::Move::createPromotion(static_cast<uint8_t>(from), static_cast<uint8_t>(to), piece, PromotionType::Bishop, capture));
        validMoves.addMove(movement::Move::createPromotion(static_cast<uint8_t>(from), static_cast<uint8_t>(to), piece, PromotionType::Rook, capture));
    }
}

}

constexpr static inline void getWhitePawnMoves(ValidMoves& validMoves, uint64_t pawns, uint64_t ownOccupation, uint64_t theirOccupation)
{
    const uint64_t allOccupation = ownOccupation | theirOccupation;

    /*
     * straight -> full row bit shift where there no other pieces
     * straightDouble -> same as straight but only second row BUT also where third row is not occupied - to compensate for this,
     * we move the black pieces a row back to block these out on a binary level
     * attackLeft -> if straight is 8 then left must be 7 (all shifted one to the side). This should never happen to a file pawns.
     * attackRight -> same as left but the other direction
     */
    uint64_t moveStraight = ((pawns & ~s_row7Mask) << 8) & ~allOccupation;
    uint64_t moveStraightDouble = ((pawns & s_row2Mask) << 16) & ~(allOccupation | (allOccupation << 8));
    uint64_t attackLeft = ((pawns & ~s_row7Mask & ~s_aFileMask) << 7) & theirOccupation;
    uint64_t attackRight = ((pawns & ~s_row7Mask & ~s_hFileMask) << 9) & theirOccupation;

    backtrackPawnMoves(validMoves, moveStraight, 8, Piece::WhitePawn, false);
    backtrackPawnMoves(validMoves, moveStraightDouble, 16, Piece::WhitePawn, false);
    backtrackPawnMoves(validMoves, attackLeft, 7, Piece::WhitePawn, true);
    backtrackPawnMoves(validMoves, attackRight, 9, Piece::WhitePawn, true);

    uint64_t promoteStraight = ((pawns & s_row7Mask) << 8) & ~allOccupation;
    uint64_t promoteAttackLeft = ((pawns & s_row7Mask & ~s_aFileMask) << 7) & theirOccupation;
    uint64_t promoteAttackRight = ((pawns & s_row7Mask & ~s_hFileMask) << 9) & theirOccupation;

    backtrackPawnPromotions(validMoves, promoteStraight, 8, Piece::WhitePawn, false);
    backtrackPawnPromotions(validMoves, promoteAttackLeft, 7, Piece::WhitePawn, true);
    backtrackPawnPromotions(validMoves, promoteAttackRight, 9, Piece::WhitePawn, true);
}

constexpr static inline void getBlackPawnMoves(ValidMoves& validMoves, uint64_t pawns, uint64_t ownOccupation, uint64_t theirOccupation)
{
    const uint64_t allOccupation = theirOccupation | ownOccupation;

    uint64_t moveStraight = ((pawns & ~s_row2Mask) >> 8) & ~allOccupation;
    uint64_t moveStraightDouble = ((pawns & s_row7Mask) >> 16) & ~(allOccupation | (allOccupation >> 8));
    uint64_t attackLeft = ((pawns & ~s_row2Mask & ~s_aFileMask) >> 9) & theirOccupation;
    uint64_t attackRight = ((pawns & ~s_row2Mask & ~s_hFileMask) >> 7) & theirOccupation;

    backtrackPawnMoves(validMoves, moveStraight, -8, Piece::BlackPawn, false);
    backtrackPawnMoves(validMoves, moveStraightDouble, -16, Piece::BlackPawn, false);
    backtrackPawnMoves(validMoves, attackLeft, -9, Piece::BlackPawn, true);
    backtrackPawnMoves(validMoves, attackRight, -7, Piece::BlackPawn, true);

    uint64_t promoteStraight = ((pawns & s_row2Mask) >> 8) & ~allOccupation;
    uint64_t promoteAttackLeft = ((pawns & s_row2Mask & ~s_aFileMask) >> 9) & theirOccupation;
    uint64_t promoteAttackRight = ((pawns & s_row2Mask & ~s_hFileMask) >> 7) & theirOccupation;

    backtrackPawnPromotions(validMoves, promoteStraight, -8, Piece::BlackPawn, false);
    backtrackPawnPromotions(validMoves, promoteAttackLeft, -9, Piece::BlackPawn, true);
    backtrackPawnPromotions(validMoves, promoteAttackRight, -7, Piece::BlackPawn, true);
}

}

