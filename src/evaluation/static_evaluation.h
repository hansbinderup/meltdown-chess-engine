#pragma once

#include "bit_board.h"
#include "evaluation/pesto_tables.h"
#include "evaluation/positioning.h"

#include <cstdint>

/*
 * Current evaluation is pretty simple.
 * It based on the tables from:
 * https://www.chessprogramming.org/Simplified_Evaluation_Function
 */

namespace evaluation {

constexpr int32_t staticEvaluation(const BitBoard& board)
{
    /* we first evaluate based on "pesto score"
     * then we add mobility, double pawn etc. */
    int32_t score = pesto::evaluate(board);

    // Material scoring
    for (const auto piece : magic_enum::enum_values<Piece>()) {
        switch (piece) {
        case WhitePawn:
            score += position::getPawnScore<PlayerWhite>(board, board.pieces[piece]);
            break;
        case WhiteKnight:
            break;
        case WhiteBishop:
            score += position::getBishopScore(board, board.pieces[piece]);
            break;
        case WhiteRook:
            score += position::getRookScore<PlayerWhite>(board, board.pieces[piece]);
            break;
        case WhiteQueen:
            score += position::getQueenScore(board, board.pieces[piece]);
            break;
        case WhiteKing:
            score += position::getKingScore<PlayerWhite>(board, board.pieces[piece]);
            break;

            /* BLACK PIECES - should all be negated! */

        case BlackPawn:
            score -= position::getPawnScore<PlayerBlack>(board, board.pieces[piece]);
            break;
        case BlackKnight:
            break;
        case BlackBishop:
            score -= position::getBishopScore(board, board.pieces[piece]);
            break;
        case BlackRook:
            score -= position::getRookScore<PlayerBlack>(board, board.pieces[piece]);
            break;
        case BlackQueen:
            score -= position::getQueenScore(board, board.pieces[piece]);
            break;
        case BlackKing:
            score -= position::getKingScore<PlayerBlack>(board, board.pieces[piece]);
            break;
        }
    }

    return board.player == PlayerWhite ? score : -score;
}

}
