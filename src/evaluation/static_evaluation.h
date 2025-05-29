#pragma once

#include "attack_generation.h"
#include "bit_board.h"
#include "evaluation/term_methods.h"

namespace evaluation {

/* helper macro to apply score for both sides - with correct scoring sign
 * positive for white
 * negative for black
 *
 * usage:
 *
 *    template<Player player>
 *    TermScore termFunc(arg1, arg2, ... argN);
 *
 *    APPLY_SCORE(termFunc, arg1, arg2, ... argN) */
#define APPLY_SCORE(func, ...)                   \
    {                                            \
        score += func<PlayerWhite>(__VA_ARGS__); \
        score -= func<PlayerBlack>(__VA_ARGS__); \
    }

static inline TermContext prepareContext(const BitBoard& board)
{
    return TermContext {
        .pawnAttacks { attackgen::getWhitePawnAttacks(board), attackgen::getBlackPawnAttacks(board) },
        .kingZone { attackgen::getKingAttacks<PlayerWhite>(board), attackgen::getKingAttacks<PlayerBlack>(board) },
        .kingAttacks { 0, 0 },
    };
}

static inline Score staticEvaluation(const BitBoard& board)
{
    TermScore score(0, 0);

    uint8_t phaseScore = 0;
    auto ctx = prepareContext(board);

    // Material scoring
    for (const auto piece : magic_enum::enum_values<Piece>()) {
        switch (piece) {
        case WhitePawn:
            score += getPawnScore<PlayerWhite>(board, ctx, board.pieces[piece]);
            break;
        case WhiteKnight:
            score += getKnightScore<PlayerWhite>(board, ctx, board.pieces[piece], phaseScore);
            break;
        case WhiteBishop:
            score += getBishopScore<PlayerWhite>(board, ctx, board.pieces[piece], phaseScore);
            break;
        case WhiteRook:
            score += getRookScore<PlayerWhite>(board, ctx, board.pieces[piece], phaseScore);
            break;
        case WhiteQueen:
            score += getQueenScore<PlayerWhite>(board, ctx, board.pieces[piece], phaseScore);
            break;
        case WhiteKing:
            score += getKingScore<PlayerWhite>(board, board.pieces[piece]);
            break;

            /* BLACK PIECES - should all be negated! */
        case BlackPawn:
            score -= getPawnScore<PlayerBlack>(board, ctx, board.pieces[piece]);
            break;
        case BlackKnight:
            score -= getKnightScore<PlayerBlack>(board, ctx, board.pieces[piece], phaseScore);
            break;
        case BlackBishop:
            score -= getBishopScore<PlayerBlack>(board, ctx, board.pieces[piece], phaseScore);
            break;
        case BlackRook:
            score -= getRookScore<PlayerBlack>(board, ctx, board.pieces[piece], phaseScore);
            break;
        case BlackQueen:
            score -= getQueenScore<PlayerBlack>(board, ctx, board.pieces[piece], phaseScore);
            break;
        case BlackKing:
            score -= getKingScore<PlayerBlack>(board, board.pieces[piece]);
            break;
        }
    }

    APPLY_SCORE(getKingZoneScore, ctx);

    const Score evaluation = score.phaseScore(phaseScore);
    return board.player == PlayerWhite ? evaluation : -evaluation;
}

}
