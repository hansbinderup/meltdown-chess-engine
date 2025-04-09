#pragma once

#include "bit_board.h"
#include "evaluation/generated/tuned_terms.h"
#include "evaluation/position_tables.h"
#include "helpers/bit_operations.h"
#include "movegen/bishops.h"
#include "movegen/kings.h"
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

[[nodiscard]] constexpr uint64_t flipPosition(BoardPosition pos) noexcept
{
    return pos ^ 56;
}

constexpr uint8_t pawnShieldSize = s_terms.pawnShieldBonus.size();

/* TODO: divide into terms instead of pieces.. */

template<Player player>
constexpr static inline Score getPawnScore(const BitBoard& board, const uint64_t pawns)
{
    Score score(0, 0);

    helper::bitIterate(pawns, [&](BoardPosition pos) {
        const uint64_t square = helper::positionToSquare(pos);

        ADD_SCORE_INDEXED(pieceValues, Pawn);

        const auto doubledPawns = std::popcount(pawns & s_fileMaskTable[pos]);
        if (doubledPawns > 1)
            ADD_SCORE(doublePawnPenalty);

        if ((pawns & s_isolationMaskTable[pos]) == 0)
            ADD_SCORE(isolatedPawnPenalty);

        if constexpr (player == PlayerWhite) {
            ADD_SCORE_INDEXED(psqtPawns, pos);

            if ((board.pieces[BlackPawn] & s_whitePassedPawnMaskTable[pos]) == 0) {
                const uint8_t row = (pos / 8);
                ADD_SCORE_INDEXED(passedPawnBonus, row);
            }

            const auto kingPos = helper::lsbToPosition(board.pieces[WhiteKing]);
            if (s_whitePassedPawnMaskTable[kingPos] & square) {
                const uint8_t shieldDistance = std::min(helper::verticalDistance(kingPos, pos), pawnShieldSize);
                ADD_SCORE_INDEXED(pawnShieldBonus, shieldDistance - 1);
            }
        } else {
            ADD_SCORE_INDEXED(psqtPawns, flipPosition(pos));

            if ((board.pieces[WhitePawn] & s_blackPassedPawnMaskTable[pos]) == 0) {
                const uint8_t row = 7 - (pos / 8);
                ADD_SCORE_INDEXED(passedPawnBonus, row);
            }

            const auto kingPos = helper::lsbToPosition(board.pieces[BlackKing]);
            if (s_blackPassedPawnMaskTable[kingPos] & square) {
                const uint8_t shieldDistance = std::min(helper::verticalDistance(kingPos, pos), pawnShieldSize);
                ADD_SCORE_INDEXED(pawnShieldBonus, shieldDistance - 1);
            }
        }
    });

    return score;
}

template<Player player>
constexpr static inline Score getKnightScore(const BitBoard& board, const uint64_t knights, uint8_t& phaseScore)
{
    Score score(0, 0);

    helper::bitIterate(knights, [&](BoardPosition pos) {
        phaseScore += s_piecePhaseValues[Knight];
        ADD_SCORE_INDEXED(pieceValues, Knight);

        const uint64_t moves = movegen::getKnightMoves(pos) & ~board.occupation[player];
        const int movesCount = std::popcount(moves);
        ADD_SCORE_INDEXED(knightMobilityScore, movesCount);

        if constexpr (player == PlayerWhite) {
            ADD_SCORE_INDEXED(psqtKnights, pos);
        } else {
            ADD_SCORE_INDEXED(psqtKnights, flipPosition(pos));
        }
    });

    return score;
}

template<Player player>
constexpr static inline Score getBishopScore(const BitBoard& board, const uint64_t bishops, uint8_t& phaseScore)
{
    Score score(0, 0);

    const int amntBishops = std::popcount(bishops);
    if (amntBishops >= 2)
        ADD_SCORE(bishopPairScore)

    helper::bitIterate(bishops, [&](BoardPosition pos) {
        phaseScore += s_piecePhaseValues[Bishop];
        ADD_SCORE_INDEXED(pieceValues, Bishop);

        const uint64_t moves = movegen::getBishopMoves(pos, board.occupation[Both]);
        const int movesCount = std::popcount(moves);
        ADD_SCORE_INDEXED(bishopMobilityScore, movesCount);

        if constexpr (player == PlayerWhite) {
            ADD_SCORE_INDEXED(psqtBishops, pos);
        } else {
            ADD_SCORE_INDEXED(psqtBishops, flipPosition(pos));
        }
    });

    return score;
}

template<Player player>
constexpr static inline Score getRookScore(const BitBoard& board, const uint64_t rooks, uint8_t& phaseScore)
{
    Score score(0, 0);

    const uint64_t whitePawns = board.pieces[WhitePawn];
    const uint64_t blackPawns = board.pieces[BlackPawn];

    helper::bitIterate(rooks, [&](BoardPosition pos) {
        phaseScore += s_piecePhaseValues[Rook];
        ADD_SCORE_INDEXED(pieceValues, Rook);

        const uint64_t moves = movegen::getRookMoves(pos, board.occupation[Both]);
        const int movesCount = std::popcount(moves);
        ADD_SCORE_INDEXED(rookMobilityScore, movesCount);

        if (((whitePawns | blackPawns) & s_fileMaskTable[pos]) == 0)
            ADD_SCORE(rookOpenFileBonus);

        if constexpr (player == PlayerWhite) {
            ADD_SCORE_INDEXED(psqtRooks, pos);
            if ((whitePawns & s_fileMaskTable[pos]) == 0)
                ADD_SCORE(rookSemiOpenFileBonus);
        } else {
            ADD_SCORE_INDEXED(psqtRooks, flipPosition(pos));
            if ((blackPawns & s_fileMaskTable[pos]) == 0)
                ADD_SCORE(rookSemiOpenFileBonus);
        }
    });

    return score;
}

template<Player player>
constexpr static inline Score getQueenScore(const BitBoard& board, const uint64_t queens, uint8_t& phaseScore)
{
    Score score(0, 0);

    const uint64_t whitePawns = board.pieces[WhitePawn];
    const uint64_t blackPawns = board.pieces[BlackPawn];

    helper::bitIterate(queens, [&](BoardPosition pos) {
        phaseScore += s_piecePhaseValues[Queen];
        ADD_SCORE_INDEXED(pieceValues, Queen);

        if (((whitePawns | blackPawns) & s_fileMaskTable[pos]) == 0)
            ADD_SCORE(queenOpenFileBonus);

        const uint64_t moves
            = movegen::getBishopMoves(pos, board.occupation[Both])
            | movegen::getRookMoves(pos, board.occupation[Both]);
        const int movesCount = std::popcount(moves);
        ADD_SCORE_INDEXED(queenMobilityScore, movesCount);

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
constexpr static inline Score getKingScore(const uint64_t king)
{
    Score score(0, 0);

    helper::bitIterate(king, [&](BoardPosition pos) {
        if constexpr (player == PlayerWhite) {
            ADD_SCORE_INDEXED(psqtKings, pos);
        } else {
            ADD_SCORE_INDEXED(psqtKings, flipPosition(pos));
        }
    });

    return score;
}

}

