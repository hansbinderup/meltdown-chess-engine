#pragma once

#include "bit_board.h"

namespace evaluation {

static inline bool positionIsDraw(const BitBoard& board)
{
    /* 50-move rule */
    if (board.halfMoves >= 100) {
        return true;
    }

    /* try to termintate as fast as possible - this will be the cast until endgame */
    const int totalPieces = std::popcount(board.occupation[Both]);
    if (totalPieces > 4) {
        return false;
    }

    /* if pawns are on the board, there is always sufficient material */
    const uint64_t pawns = board.pieces[WhitePawn] | board.pieces[BlackPawn];
    if (std::popcount(pawns) != 0) {
        return false;
    }

    if (totalPieces <= 2) {
        /* only kings on the board */
        return true;
    }

    if (totalPieces == 3) {
        const uint64_t minors = board.pieces[WhiteBishop] | board.pieces[BlackBishop] | board.pieces[WhiteKnight] | board.pieces[BlackKnight];

        /* one minor piece + kings is still a draw */
        return std::popcount(minors) == 1;
    }

    if (totalPieces == 4) {
        const uint64_t bishops = board.pieces[WhiteBishop] | board.pieces[BlackBishop];

        /* check for exactly one bishop per side */
        const bool oneWhiteBishop = std::popcount(board.pieces[WhiteBishop]) == 1;

        /* check if bishops are on opposite color squares */
        const bool bishopOnLight = std::popcount(s_lightSquares & bishops) == 1;
        const bool bishopOnDark = std::popcount(s_darkSquares & bishops) == 1;

        return oneWhiteBishop && bishopOnLight && bishopOnDark;
    }

    /* all other cases have potential for sufficient material */
    return false;
}

}

