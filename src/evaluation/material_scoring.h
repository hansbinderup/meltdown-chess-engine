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
constexpr static inline auto s_pieceScoring = std::to_array<int32_t>({
    100, /* White Pawn */
    300, /* White Knight */
    350, /* White Bishop */
    500, /* White Rook */
    1000, /* White Queen */
    10000, /* White King */
    -100, /* Black Pawn */
    -300, /* Black Knight */
    -350, /* Black Bishop */
    -500, /* Black Rook */
    -1000, /* Black Queen */
    -10000, /* Black King */
});

}

constexpr int32_t materialScore(const BitBoard& board)
{
    int32_t score = 0;

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

    score += getPawnScore<PlayerWhite>(board.pieces[WhitePawn]);
    score -= getPawnScore<PlayerBlack>(board.pieces[BlackPawn]);

    return board.player == PlayerWhite ? score : -score;
}

}
