#pragma once

#include "movegen/move_types.h"
#include "utils/bit_operations.h"

#include <cstdint>

namespace movegen {

namespace {

constexpr void backtrackPawnMoves(ValidMoves& validMoves, uint64_t moves, int8_t bitCnt, bool capture)
{

    utils::bitIterate(moves, [&](BoardPosition to) {
        const int from = to - bitCnt; // Backtrack the pawn's original position
        validMoves.addMove(movegen::Move::create(from, to, capture));
    });
}

constexpr void backtrackPawnEnPessantMoves(ValidMoves& validMoves, uint64_t moves, int8_t bitCnt, bool doublePush)
{

    utils::bitIterate(moves, [&](BoardPosition to) {
        const int from = to - bitCnt; // Backtrack the pawn's original position
        validMoves.addMove(movegen::Move::createEnPessant(from, to, doublePush));
    });
}

constexpr void backtrackPawnPromotions(ValidMoves& validMoves, uint64_t moves, int8_t bitCnt, bool capture)
{
    utils::bitIterate(moves, [&](BoardPosition to) {
        const int from = to - bitCnt; // Backtrack the pawn's original position

        /* add queen first - is usually the preferred piece */
        validMoves.addMove(movegen::Move::createPromotion(from, to, PromotionQueen, capture));
        validMoves.addMove(movegen::Move::createPromotion(from, to, PromotionKnight, capture));
        validMoves.addMove(movegen::Move::createPromotion(from, to, PromotionBishop, capture));
        validMoves.addMove(movegen::Move::createPromotion(from, to, PromotionRook, capture));
    });
}

}

template<movegen::MoveType type>
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

    if constexpr (type == MovePseudoLegal) {
        uint64_t moveStraight = ((pawns & ~s_row7Mask) << 8) & ~allOccupation;
        uint64_t moveStraightDouble = ((pawns & s_row2Mask) << 16) & ~(allOccupation | (allOccupation << 8));
        backtrackPawnMoves(validMoves, moveStraight, 8, false);
        backtrackPawnEnPessantMoves(validMoves, moveStraightDouble, 16, true);
    }

    uint64_t attackLeft = ((pawns & ~s_row7Mask & ~s_aFileMask) << 7) & theirOccupation;
    uint64_t attackRight = ((pawns & ~s_row7Mask & ~s_hFileMask) << 9) & theirOccupation;

    backtrackPawnMoves(validMoves, attackLeft, 7, true);
    backtrackPawnMoves(validMoves, attackRight, 9, true);

    if constexpr (type == MovePseudoLegal || type == MoveNoisy) {
        uint64_t promoteStraight = ((pawns & s_row7Mask) << 8) & ~allOccupation;
        backtrackPawnPromotions(validMoves, promoteStraight, 8, false);
    }

    uint64_t promoteAttackLeft = ((pawns & s_row7Mask & ~s_aFileMask) << 7) & theirOccupation;
    uint64_t promoteAttackRight = ((pawns & s_row7Mask & ~s_hFileMask) << 9) & theirOccupation;

    backtrackPawnPromotions(validMoves, promoteAttackLeft, 7, true);
    backtrackPawnPromotions(validMoves, promoteAttackRight, 9, true);
}

template<movegen::MoveType type>
constexpr static inline void getBlackPawnMoves(ValidMoves& validMoves, uint64_t pawns, uint64_t ownOccupation, uint64_t theirOccupation)
{
    const uint64_t allOccupation = theirOccupation | ownOccupation;

    if constexpr (type == MovePseudoLegal) {
        uint64_t moveStraight = ((pawns & ~s_row2Mask) >> 8) & ~allOccupation;
        uint64_t moveStraightDouble = ((pawns & s_row7Mask) >> 16) & ~(allOccupation | (allOccupation >> 8));

        backtrackPawnMoves(validMoves, moveStraight, -8, false);
        backtrackPawnEnPessantMoves(validMoves, moveStraightDouble, -16, true);
    }

    uint64_t attackLeft = ((pawns & ~s_row2Mask & ~s_aFileMask) >> 9) & theirOccupation;
    uint64_t attackRight = ((pawns & ~s_row2Mask & ~s_hFileMask) >> 7) & theirOccupation;

    backtrackPawnMoves(validMoves, attackLeft, -9, true);
    backtrackPawnMoves(validMoves, attackRight, -7, true);

    if constexpr (type == MovePseudoLegal || type == MoveNoisy) {
        uint64_t promoteStraight = ((pawns & s_row2Mask) >> 8) & ~allOccupation;

        backtrackPawnPromotions(validMoves, promoteStraight, -8, false);
    }

    uint64_t promoteAttackLeft = ((pawns & s_row2Mask & ~s_aFileMask) >> 9) & theirOccupation;
    uint64_t promoteAttackRight = ((pawns & s_row2Mask & ~s_hFileMask) >> 7) & theirOccupation;

    backtrackPawnPromotions(validMoves, promoteAttackLeft, -9, true);
    backtrackPawnPromotions(validMoves, promoteAttackRight, -7, true);
}

constexpr static inline void getWhiteEnPessantMoves(ValidMoves& validMoves, uint64_t pawns, BoardPosition enPessant, uint64_t occupation)
{
    const uint64_t enPessantSquare = utils::positionToSquare(enPessant);

    // TODO: masks row 5?
    uint64_t moveLeft = ((pawns & ~s_aFileMask) << 7) & enPessantSquare & ~occupation;
    uint64_t moveRight = ((pawns & ~s_hFileMask) << 9) & enPessantSquare & ~occupation;

    backtrackPawnEnPessantMoves(validMoves, moveLeft, 7, false);
    backtrackPawnEnPessantMoves(validMoves, moveRight, 9, false);
}

constexpr static inline void getBlackEnPessantMoves(ValidMoves& validMoves, uint64_t pawns, BoardPosition enPessant, uint64_t occupation)
{
    const uint64_t enPessantSquare = utils::positionToSquare(enPessant);

    // TODO: masks row 4?
    uint64_t moveLeft = ((pawns & ~s_aFileMask) >> 9) & enPessantSquare & ~occupation;
    uint64_t moveRight = ((pawns & ~s_hFileMask) >> 7) & enPessantSquare & ~occupation;

    backtrackPawnEnPessantMoves(validMoves, moveLeft, -9, false);
    backtrackPawnEnPessantMoves(validMoves, moveRight, -7, false);
}

/* returns a bitmask representing all squares attacked by pawns of the given player
 * from the specified bitboard `pawns`. Each bit set in `pawns` is treated as a pawn's position
 *
 * note: This function does not account for en passant captures */
template<Player player>
static inline uint64_t getPawnAttacks(uint64_t pawns)
{
    if constexpr (player == PlayerWhite) {
        const uint64_t attackLeft = (pawns & ~(s_aFileMask | s_row8Mask)) << 7;
        const uint64_t attackRight = (pawns & ~(s_hFileMask | s_row8Mask)) << 9;

        return attackLeft | attackRight;
    } else {
        const uint64_t attackLeft = (pawns & ~(s_aFileMask | s_row1Mask)) >> 9;
        const uint64_t attackRight = (pawns & ~(s_hFileMask | s_row1Mask)) >> 7;

        return attackLeft | attackRight;
    }
}

/* helper to compuate bitmask of squares attacked by a pawn of the given player from a single `BoardPosition` */
template<Player player>
static inline uint64_t getPawnAttacksFromPos(BoardPosition pos)
{
    const uint64_t square = utils::positionToSquare(pos);
    return getPawnAttacks<player>(square);
}

}
