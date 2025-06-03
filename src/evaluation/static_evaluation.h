#pragma once

#include "core/attack_generation.h"
#include "core/bit_board.h"
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
        .attacksToKingZone { 0, 0 },
        .pieceAttacks {},
        .threats { 0, 0 },
    };
}

static inline Score staticEvaluation(const BitBoard& board)
{
    TermScore score(0, 0);

    uint8_t phaseScore = 0;
    auto ctx = prepareContext(board);

    /* piece scores - should be computed first as they populate ctx */
    APPLY_SCORE(getPawnScore, board, ctx);
    APPLY_SCORE(getKnightScore, board, ctx, phaseScore);
    APPLY_SCORE(getBishopScore, board, ctx, phaseScore);
    APPLY_SCORE(getRookScore, board, ctx, phaseScore);
    APPLY_SCORE(getQueenScore, board, ctx, phaseScore);
    APPLY_SCORE(getKingScore, board, ctx);

    /* terms that consume ctx */
    APPLY_SCORE(getKingZoneScore, ctx);
    APPLY_SCORE(getPieceAttacksScore, board, ctx);
    APPLY_SCORE(getChecksScore, board, ctx);
    APPLY_SCORE(getPawnPushThreatScore, board, ctx);

    const Score evaluation = score.phaseScore(phaseScore);
    return board.player == PlayerWhite ? evaluation : -evaluation;
}

}
