#pragma once

#include "core/attack_generation.h"
#include "core/bit_board.h"
#include "evaluation/kingPawnCache.h"
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

class StaticEvaluation {
public:
    /* computes the static evaluation for the given board position
     * value is scaled realtive to game phase resulting in a single score */
    Score get(const BitBoard& board)
    {
        TermScore score(0, 0);

        uint8_t phaseScore = 0;
        auto ctx = prepareContext(board);

        /* terms that are not using ctx */
        APPLY_SCORE(getTempoScore, board);

        /* when tuning, we should never try to use the cache - we need to always update the tracer */
#ifdef TUNING
        APPLY_SCORE(getStaticKingPawnScore, board, ctx);
#else
        score += fetchOrStoreKpCache(board, ctx);
#endif

        /* piece scores - should be computed first as they populate ctx */
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
        APPLY_SCORE(getPassedPawnsScore, board, ctx);

        const Score evaluation = score.phaseScore(phaseScore);
        return board.player == PlayerWhite ? evaluation : -evaluation;
    }

private:
    TermContext prepareContext(const BitBoard& board) const
    {
        auto ctx = TermContext {
            .pawnAttacks { attackgen::getPawnAttacks<PlayerWhite>(board), attackgen::getPawnAttacks<PlayerBlack>(board) },
            .kingZone { attackgen::getKingAttacks<PlayerWhite>(board), attackgen::getKingAttacks<PlayerBlack>(board) },
            .attacksToKingZone { 0, 0 },
            .pieceAttacks {},
            .threats { 0, 0 },
            .passedPawns { 0, 0 },
        };

        ctx.attacksToKingZone[PlayerWhite] += std::popcount(ctx.kingZone[PlayerWhite] & ctx.pawnAttacks[PlayerBlack]);
        ctx.attacksToKingZone[PlayerBlack] += std::popcount(ctx.kingZone[PlayerBlack] & ctx.pawnAttacks[PlayerWhite]);

        ctx.pieceAttacks[PlayerWhite][Pawn] = ctx.pawnAttacks[PlayerWhite];
        ctx.pieceAttacks[PlayerBlack][Pawn] = ctx.pawnAttacks[PlayerBlack];

        ctx.threats[PlayerWhite] |= ctx.pawnAttacks[PlayerWhite];
        ctx.threats[PlayerBlack] |= ctx.pawnAttacks[PlayerBlack];

        return ctx;
    }

    TermScore fetchOrStoreKpCache(const BitBoard& board, TermContext& ctx)
    {
        TermScore score(0, 0);

        const auto kpProbe = m_kpCache.probe(board.kpHash);
        if (kpProbe.has_value()) {
            score += kpProbe->score;
            ctx.passedPawns = kpProbe->passedPawns;
        } else {
            APPLY_SCORE(getStaticKingPawnScore, board, ctx);
            m_kpCache.write(KingPawnEntry { .key = board.kpHash, .score = score, .passedPawns = ctx.passedPawns });
        }

        return score;
    }

    KingPawnCache m_kpCache {};
};

}
