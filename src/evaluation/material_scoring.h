#pragma once

#include "src/bit_board.h"
#include "src/evaluation/positioning.h"

#include <cstdint>

/*
 * Current evaluation is pretty simple.
 * It based on the tables from:
 * https://www.chessprogramming.org/Simplified_Evaluation_Function
 */

namespace evaluation {

namespace {

/* material score for each piece - lookup based on Piece enum */
constexpr static inline auto s_pieceScoring = std::to_array<int16_t>({
    100, /* White Pawn */
    320, /* White Knight */
    330, /* White Bishop */
    500, /* White Rook */
    900, /* White Queen */
    20000, /* White King */
    -100, /* Black Pawn */
    -320, /* Black Knight */
    -330, /* Black Bishop */
    -500, /* Black Rook */
    -900, /* Black Queen */
    -20000, /* Black King */
});

}

constexpr int16_t materialScore(const BitBoard& board)
{
    int16_t score = 0;

    // Material scoring
    for (const auto piece : magic_enum::enum_values<Piece>()) {
        score += std::popcount(board.pieces[piece]) * s_pieceScoring[piece];
    }

    for (const auto piece : s_whitePieces) {
        score += getPiecePositionScore(board.pieces[piece], piece);
    }

    for (const auto piece : s_blackPieces) {
        score -= getPiecePositionScore(board.pieces[piece], piece);
    }

    // bishop pairs are worth extra
    if (std::popcount(board.pieces[WhiteBishop]) >= 2)
        score += 150;

    if (std::popcount(board.pieces[BlackBishop]) >= 2)
        score -= 150;

    // knights are worth more if there are many pawns
    if (std::popcount(board.pieces[BlackPawn]) >= 5)
        score += std::popcount(board.pieces[WhiteKnight]) * 30;

    if (std::popcount(board.pieces[WhitePawn]) >= 5)
        score -= std::popcount(board.pieces[BlackKnight]) * 30;

    return board.player == Player::White ? score : -score;
}

}
