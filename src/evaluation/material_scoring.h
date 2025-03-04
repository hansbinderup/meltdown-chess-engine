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

        switch (piece) {
        case WhitePawn:
            score += position::getPawnScore<PlayerWhite>(board, board.pieces[piece]);
            break;
        case WhiteKnight:
            score += position::getKnightScore<PlayerWhite>(board.pieces[piece]);
            break;
        case WhiteBishop:
            score += position::getBishopScore<PlayerWhite>(board, board.pieces[piece]);
            break;
        case WhiteRook:
            score += position::getRookScore<PlayerWhite>(board, board.pieces[piece]);
            break;
        case WhiteQueen:
            score += position::getQueenScore<PlayerWhite>(board, board.pieces[piece]);
            break;
        case WhiteKing:
            score += position::getKingScore<PlayerWhite>(board, board.pieces[piece]);
            break;

            /* BLACK PIECES - should all be negated! */

        case BlackPawn:
            score -= position::getPawnScore<PlayerBlack>(board, board.pieces[piece]);
            break;
        case BlackKnight:
            score -= position::getKnightScore<PlayerBlack>(board.pieces[piece]);
            break;
        case BlackBishop:
            score -= position::getBishopScore<PlayerBlack>(board, board.pieces[piece]);
            break;
        case BlackRook:
            score -= position::getRookScore<PlayerBlack>(board, board.pieces[piece]);
            break;
        case BlackQueen:
            score -= position::getQueenScore<PlayerBlack>(board, board.pieces[piece]);
            break;
        case BlackKing:
            score -= position::getKingScore<PlayerBlack>(board, board.pieces[piece]);
            break;
        }
    }

    return board.player == PlayerWhite ? score : -score;
}

}
