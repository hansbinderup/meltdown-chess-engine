#pragma once

#include "bit_board.h"
#include "evaluation/term_methods.h"

#include <cstdint>

namespace evaluation {

constexpr int32_t staticEvaluation(const BitBoard& board)
{
    TermScore score(0, 0);
    uint8_t phaseScore {};

    // Material scoring
    for (const auto piece : magic_enum::enum_values<Piece>()) {
        switch (piece) {
        case WhitePawn:
            score += getPawnScore<PlayerWhite>(board, board.pieces[piece]);
            break;
        case WhiteKnight:
            score += getKnightScore<PlayerWhite>(board, board.pieces[piece], phaseScore);
            break;
        case WhiteBishop:
            score += getBishopScore<PlayerWhite>(board, board.pieces[piece], phaseScore);
            break;
        case WhiteRook:
            score += getRookScore<PlayerWhite>(board, board.pieces[piece], phaseScore);
            break;
        case WhiteQueen:
            score += getQueenScore<PlayerWhite>(board, board.pieces[piece], phaseScore);
            break;
        case WhiteKing:
            score += getKingScore<PlayerWhite>(board, board.pieces[piece]);
            break;

            /* BLACK PIECES - should all be negated! */

        case BlackPawn:
            score -= getPawnScore<PlayerBlack>(board, board.pieces[piece]);
            break;
        case BlackKnight:
            score -= getKnightScore<PlayerBlack>(board, board.pieces[piece], phaseScore);
            break;
        case BlackBishop:
            score -= getBishopScore<PlayerBlack>(board, board.pieces[piece], phaseScore);
            break;
        case BlackRook:
            score -= getRookScore<PlayerBlack>(board, board.pieces[piece], phaseScore);
            break;
        case BlackQueen:
            score -= getQueenScore<PlayerBlack>(board, board.pieces[piece], phaseScore);
            break;
        case BlackKing:
            score -= getKingScore<PlayerBlack>(board, board.pieces[piece]);
            break;
        }
    }

    const int32_t evaluation = score.phaseScore(phaseScore);
    return board.player == PlayerWhite ? evaluation : -evaluation;
}

}
