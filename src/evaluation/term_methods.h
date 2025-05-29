#pragma once

#include "bit_board.h"
#include "evaluation/generated/tuned_terms.h"
#include "evaluation/position_tables.h"
#include "helpers/bit_operations.h"
#include "movegen/bishops.h"
#include "movegen/knights.h"
#include "movegen/rooks.h"

#include <array>

#ifdef TUNING

#include "tuner/term_tracer.h"

#define ADD_SCORE_INDEXED(weightName, index) \
    {                                        \
        score += s_terms.weightName[index];  \
        if constexpr (player == PlayerWhite) \
            s_trace.weightName[index]++;     \
        else                                 \
            s_trace.weightName[index]--;     \
    }

#else

#define ADD_SCORE_INDEXED(weightName, index) \
    {                                        \
        score += s_terms.weightName[index];  \
    }

#endif

/* helper for single index tables */
#define ADD_SCORE(weightName) ADD_SCORE_INDEXED(weightName, 0)

namespace evaluation {

/* A context to precompute heavy operations so we don't have to do that
 * multiple times for a single evaluation
 * NOTE: Only values that are being reused between terms should be added here */
struct TermContext {
    const std::array<uint64_t, magic_enum::enum_count<Player>()> pawnAttacks;
    const std::array<uint64_t, magic_enum::enum_count<Player>()> kingZone;

    std::array<uint8_t, magic_enum::enum_count<Player>()> kingAttacks;
};

[[nodiscard]] constexpr uint64_t flipPosition(BoardPosition pos) noexcept
{
    return pos ^ 56;
}

constexpr uint8_t pawnShieldSize = s_terms.pawnShieldBonus.size();
constexpr uint8_t kingZoneSize = s_terms.kingZone.size();

/* TODO: divide into terms instead of pieces.. */

template<Player player>
static inline TermScore getPawnScore(const BitBoard& board, TermContext& ctx, const uint64_t pawns)
{
    constexpr Piece ourKing = player == PlayerWhite ? WhiteKing : BlackKing;
    constexpr Player opponent = nextPlayer(player);

    TermScore score(0, 0);

    /* fast and simple way to compute pawns attacking their king zone */
    ctx.kingAttacks[opponent] += std::popcount(ctx.kingZone[opponent] & ctx.pawnAttacks[player]);

    helper::bitIterate(pawns, [&](BoardPosition pos) {
        const uint64_t square = helper::positionToSquare(pos);

        ADD_SCORE_INDEXED(pieceValues, Pawn);

        const auto doubledPawns = std::popcount(pawns & s_fileMaskTable[pos]);
        if (doubledPawns > 1)
            ADD_SCORE(doublePawnPenalty);

        if ((pawns & s_isolationMaskTable[pos]) == 0)
            ADD_SCORE(isolatedPawnPenalty);

        const auto kingPos = helper::lsbToPosition(board.pieces[ourKing]);
        if (s_passedPawnMaskTable[player][kingPos] & square) {
            const uint8_t shieldDistance = std::min(helper::verticalDistance(kingPos, pos), pawnShieldSize);
            ADD_SCORE_INDEXED(pawnShieldBonus, shieldDistance - 1);
        }

        if constexpr (player == PlayerWhite) {
            ADD_SCORE_INDEXED(psqtPawns, pos);

            if ((board.pieces[BlackPawn] & s_passedPawnMaskTable[player][pos]) == 0) {
                const uint8_t row = (pos / 8);
                ADD_SCORE_INDEXED(passedPawnBonus, row);
            }

        } else {
            ADD_SCORE_INDEXED(psqtPawns, flipPosition(pos));

            if ((board.pieces[WhitePawn] & s_passedPawnMaskTable[player][pos]) == 0) {
                const uint8_t row = 7 - (pos / 8);
                ADD_SCORE_INDEXED(passedPawnBonus, row);
            }
        }
    });

    return score;
}

template<Player player>
static inline TermScore getKnightScore(const BitBoard& board, TermContext& ctx, const uint64_t knights, uint8_t& phaseScore)
{
    TermScore score(0, 0);

    constexpr Player opponent = nextPlayer(player);
    const uint64_t theirPawnAttacks = ctx.pawnAttacks[opponent];

    const uint64_t whitePawns = board.pieces[WhitePawn];
    const uint64_t blackPawns = board.pieces[BlackPawn];
    const uint64_t pawnDefends = ctx.pawnAttacks[player];

    helper::bitIterate(knights, [&](BoardPosition pos) {
        const uint64_t moves = movegen::getKnightMoves(pos);
        const uint64_t square = helper::positionToSquare(pos);

        phaseScore += s_piecePhaseValues[Knight];
        ADD_SCORE_INDEXED(pieceValues, Knight);

        /* update mobility score based on possible moves that are not attacked by their pawns */
        const int mobilityCount = std::popcount(moves & ~board.occupation[player] & ~theirPawnAttacks);
        ADD_SCORE_INDEXED(knightMobilityScore, mobilityCount);

        /* moves into opponent king zone -> update potential king attacks */
        ctx.kingAttacks[opponent] += std::popcount(moves & ctx.kingZone[opponent]);

        if constexpr (player == PlayerWhite) {
            ADD_SCORE_INDEXED(psqtKnights, pos);

            if (!(s_outpostSquareMaskTable[player][pos] & blackPawns) && square & s_whiteOutpostRankMask) {
                const bool isOutside = square & (s_aFileMask | s_hFileMask);
                const bool isDefended = square & pawnDefends;

                ADD_SCORE_INDEXED(knightOutpostScore, isOutside + (isDefended << 1));
            }

        } else {
            ADD_SCORE_INDEXED(psqtKnights, flipPosition(pos));

            if (!(s_outpostSquareMaskTable[player][pos] & whitePawns) && square & s_blackOutpostRankMask) {
                const bool isOutside = square & (s_aFileMask | s_hFileMask);
                const bool isDefended = square & pawnDefends;

                ADD_SCORE_INDEXED(knightOutpostScore, isOutside + (isDefended << 1));
            }
        }
    });

    return score;
}

template<Player player>
static inline TermScore getBishopScore(const BitBoard& board, TermContext& ctx, const uint64_t bishops, uint8_t& phaseScore)
{
    TermScore score(0, 0);

    constexpr Player opponent = nextPlayer(player);
    const uint64_t theirPawnAttacks = ctx.pawnAttacks[opponent];

    const uint64_t whitePawns = board.pieces[WhitePawn];
    const uint64_t blackPawns = board.pieces[BlackPawn];
    const uint64_t pawnDefends = ctx.pawnAttacks[player];

    const int amntBishops = std::popcount(bishops);
    if (amntBishops >= 2)
        ADD_SCORE(bishopPairScore)

    helper::bitIterate(bishops, [&](BoardPosition pos) {
        const uint64_t moves = movegen::getBishopMoves(pos, board.occupation[Both]);
        const uint64_t square = helper::positionToSquare(pos);

        phaseScore += s_piecePhaseValues[Bishop];
        ADD_SCORE_INDEXED(pieceValues, Bishop);

        /* update mobility score based on possible moves that are not attacked by their pawns */
        const int mobilityCount = std::popcount(moves & ~board.occupation[player] & ~theirPawnAttacks);
        ADD_SCORE_INDEXED(bishopMobilityScore, mobilityCount);

        /* moves into opponent king zone -> update potential king attacks */
        ctx.kingAttacks[opponent] += std::popcount(moves & ctx.kingZone[opponent]);

        if constexpr (player == PlayerWhite) {
            ADD_SCORE_INDEXED(psqtBishops, pos);

            if (!(s_outpostSquareMaskTable[player][pos] & blackPawns) && square & s_whiteOutpostRankMask) {
                const bool isOutside = square & (s_aFileMask | s_hFileMask);
                const bool isDefended = square & pawnDefends;

                ADD_SCORE_INDEXED(bishopOutpostScore, isOutside + (isDefended << 1));
            }
        } else {
            ADD_SCORE_INDEXED(psqtBishops, flipPosition(pos));

            if (!(s_outpostSquareMaskTable[player][pos] & whitePawns) && square & s_blackOutpostRankMask) {
                const bool isOutside = square & (s_aFileMask | s_hFileMask);
                const bool isDefended = square & pawnDefends;

                ADD_SCORE_INDEXED(bishopOutpostScore, isOutside + (isDefended << 1));
            }
        }
    });

    return score;
}

template<Player player>
static inline TermScore getRookScore(const BitBoard& board, TermContext& ctx, const uint64_t rooks, uint8_t& phaseScore)
{
    TermScore score(0, 0);

    constexpr Player opponent = nextPlayer(player);
    const uint64_t theirPawnAttacks = ctx.pawnAttacks[opponent];

    const uint64_t whitePawns = board.pieces[WhitePawn];
    const uint64_t blackPawns = board.pieces[BlackPawn];

    const uint64_t whiteKing = board.pieces[WhiteKing];
    const uint64_t blackKing = board.pieces[BlackKing];

    helper::bitIterate(rooks, [&](BoardPosition pos) {
        const uint64_t moves = movegen::getRookMoves(pos, board.occupation[Both]);

        phaseScore += s_piecePhaseValues[Rook];
        ADD_SCORE_INDEXED(pieceValues, Rook);

        /* update mobility score based on possible moves that are not attacked by their pawns */
        const int mobilityCount = std::popcount(moves & ~board.occupation[player] & ~theirPawnAttacks);
        ADD_SCORE_INDEXED(rookMobilityScore, mobilityCount);

        /* moves into opponent king zone -> update potential king attacks */
        ctx.kingAttacks[opponent] += std::popcount(moves & ctx.kingZone[opponent]);

        if (((whitePawns | blackPawns) & s_fileMaskTable[pos]) == 0)
            ADD_SCORE(rookOpenFileBonus);

        if constexpr (player == PlayerWhite) {
            ADD_SCORE_INDEXED(psqtRooks, pos);
            if ((whitePawns & s_fileMaskTable[pos]) == 0)
                ADD_SCORE(rookSemiOpenFileBonus);

            if (rooks & s_row7Mask) {
                if ((blackPawns & s_row7Mask) || (blackKing & s_row8Mask)) {
                    ADD_SCORE(rook7thRankBonus);
                }
            }

        } else {
            ADD_SCORE_INDEXED(psqtRooks, flipPosition(pos));
            if ((blackPawns & s_fileMaskTable[pos]) == 0)
                ADD_SCORE(rookSemiOpenFileBonus);

            if (rooks & s_row2Mask) {
                if ((whitePawns & s_row2Mask) || (whiteKing & s_row1Mask)) {
                    ADD_SCORE(rook7thRankBonus);
                }
            }
        }
    });

    return score;
}

template<Player player>
static inline TermScore getQueenScore(const BitBoard& board, TermContext& ctx, const uint64_t queens, uint8_t& phaseScore)
{
    TermScore score(0, 0);

    constexpr Player opponent = nextPlayer(player);
    const uint64_t theirPawnAttacks = ctx.pawnAttacks[opponent];

    const uint64_t whitePawns = board.pieces[WhitePawn];
    const uint64_t blackPawns = board.pieces[BlackPawn];

    helper::bitIterate(queens, [&](BoardPosition pos) {
        const uint64_t moves = movegen::getBishopMoves(pos, board.occupation[Both]) | movegen::getRookMoves(pos, board.occupation[Both]);

        phaseScore += s_piecePhaseValues[Queen];
        ADD_SCORE_INDEXED(pieceValues, Queen);

        if (((whitePawns | blackPawns) & s_fileMaskTable[pos]) == 0)
            ADD_SCORE(queenOpenFileBonus);

        /* update mobility score based on possible moves that are not attacked by their pawns */
        const int mobilityCount = std::popcount(moves & ~board.occupation[player] & ~theirPawnAttacks);
        ADD_SCORE_INDEXED(queenMobilityScore, mobilityCount);

        /* moves into opponent king zone -> update potential king attacks */
        ctx.kingAttacks[opponent] += std::popcount(moves & ctx.kingZone[opponent]);

        if constexpr (player == PlayerWhite) {
            ADD_SCORE_INDEXED(psqtQueens, pos);
            if ((whitePawns & s_fileMaskTable[pos]) == 0)
                ADD_SCORE(queenSemiOpenFileBonus);
        } else {
            ADD_SCORE_INDEXED(psqtQueens, flipPosition(pos));
            if ((blackPawns & s_fileMaskTable[pos]) == 0)
                ADD_SCORE(queenSemiOpenFileBonus);
        }
    });

    return score;
}

template<Player player>
static inline TermScore getKingScore(const BitBoard& board, const uint64_t king)
{
    TermScore score(0, 0);

    helper::bitIterate(king, [&](BoardPosition pos) {
        /* virtual mobility - replace king with queen to see potential attacks for sliding pieces */
        const uint64_t virtualMoves
            = (movegen::getBishopMoves(pos, board.occupation[Both]) | movegen::getRookMoves(pos, board.occupation[Both]))
            & ~board.occupation[player];
        const int virtualMovesCount = std::popcount(virtualMoves);
        ADD_SCORE_INDEXED(kingVirtualMobilityScore, virtualMovesCount);

        if constexpr (player == PlayerWhite) {
            ADD_SCORE_INDEXED(psqtKings, pos);
        } else {
            ADD_SCORE_INDEXED(psqtKings, flipPosition(pos));
        }
    });

    return score;
}

template<Player player>
static inline TermScore getKingZoneScore(TermContext& ctx)
{
    TermScore score(0, 0);

    const uint8_t kingZoneCount = std::min<uint8_t>(ctx.kingAttacks[player], kingZoneSize - 1);
    ADD_SCORE_INDEXED(kingZone, kingZoneCount);

    return score;
}

}
