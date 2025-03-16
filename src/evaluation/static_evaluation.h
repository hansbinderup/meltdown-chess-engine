#pragma once

#include "bit_board.h"
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
    gamephase::Score score;

    // Material scoring
    for (const auto piece : magic_enum::enum_values<Piece>()) {
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

    const int32_t evaluation = gamephase::taperedScore(score);
    return board.player == PlayerWhite ? evaluation : -evaluation;
}

}
