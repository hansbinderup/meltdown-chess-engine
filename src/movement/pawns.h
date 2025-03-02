#pragma once

#include "src/movement/move_types.h"

#include <cstdint>

namespace movement {

namespace {

constexpr void backtrackPawnMoves(ValidMoves& validMoves, uint64_t moves, int8_t bitCnt, Piece piece, bool capture)
{
    while (moves) {
        int to = std::countr_zero(moves); // Find first set bit
        moves &= moves - 1; // Clear bit
        int from = to - bitCnt; // Backtrack the pawn's original position
        validMoves.addMove(movement::Move::create(from, to, piece, capture));
    }
}

constexpr void backtrackPawnEnPessantMoves(ValidMoves& validMoves, uint64_t moves, int8_t bitCnt, Piece piece, bool enable)
{
    while (moves) {
        int to = std::countr_zero(moves); // Find first set bit
        moves &= moves - 1; // Clear bit
        int from = to - bitCnt; // Backtrack the pawn's original position
        validMoves.addMove(movement::Move::createEnPessant(from, to, piece, enable, !enable));
    }
}

constexpr void backtrackPawnPromotions(ValidMoves& validMoves, uint64_t moves, int8_t bitCnt, Piece piece, bool capture)
{
    while (moves) {
        int to = std::countr_zero(moves); // Find first set bit
        moves &= moves - 1; // Clear bit
        int from = to - bitCnt; // Backtrack the pawn's original position

        /* add queen first - is usually the preferred piece */
        validMoves.addMove(movement::Move::createPromotion(from, to, piece, PromotionQueen, capture));
        validMoves.addMove(movement::Move::createPromotion(from, to, piece, PromotionKnight, capture));
        validMoves.addMove(movement::Move::createPromotion(from, to, piece, PromotionBishop, capture));
        validMoves.addMove(movement::Move::createPromotion(from, to, piece, PromotionRook, capture));
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
    backtrackPawnEnPessantMoves(validMoves, moveStraightDouble, 16, Piece::WhitePawn, true);
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
    backtrackPawnEnPessantMoves(validMoves, moveStraightDouble, -16, Piece::BlackPawn, true);
    backtrackPawnMoves(validMoves, attackLeft, -9, Piece::BlackPawn, true);
    backtrackPawnMoves(validMoves, attackRight, -7, Piece::BlackPawn, true);

    uint64_t promoteStraight = ((pawns & s_row2Mask) >> 8) & ~allOccupation;
    uint64_t promoteAttackLeft = ((pawns & s_row2Mask & ~s_aFileMask) >> 9) & theirOccupation;
    uint64_t promoteAttackRight = ((pawns & s_row2Mask & ~s_hFileMask) >> 7) & theirOccupation;

    backtrackPawnPromotions(validMoves, promoteStraight, -8, Piece::BlackPawn, false);
    backtrackPawnPromotions(validMoves, promoteAttackLeft, -9, Piece::BlackPawn, true);
    backtrackPawnPromotions(validMoves, promoteAttackRight, -7, Piece::BlackPawn, true);
}

constexpr static inline void getWhiteEnPessantMoves(ValidMoves& validMoves, uint64_t pawns, BoardPosition enPessant, uint64_t occupation)
{
    const uint64_t enPessantSquare = 1ULL << enPessant;

    // TODO: masks row 5?
    uint64_t moveLeft = ((pawns & ~s_aFileMask) << 7) & enPessantSquare & ~occupation;
    uint64_t moveRight = ((pawns & ~s_hFileMask) << 9) & enPessantSquare & ~occupation;

    backtrackPawnEnPessantMoves(validMoves, moveLeft, 7, Piece::WhitePawn, false);
    backtrackPawnEnPessantMoves(validMoves, moveRight, 9, Piece::WhitePawn, false);
}

constexpr static inline void getBlackEnPessantMoves(ValidMoves& validMoves, uint64_t pawns, BoardPosition enPessant, uint64_t occupation)
{
    const uint64_t enPessantSquare = 1ULL << enPessant;

    // TODO: masks row 4?
    uint64_t moveLeft = ((pawns & ~s_aFileMask) >> 9) & enPessantSquare & ~occupation;
    uint64_t moveRight = ((pawns & ~s_hFileMask) >> 7) & enPessantSquare & ~occupation;

    backtrackPawnEnPessantMoves(validMoves, moveLeft, -9, Piece::BlackPawn, false);
    backtrackPawnEnPessantMoves(validMoves, moveRight, -7, Piece::BlackPawn, false);
}

}

