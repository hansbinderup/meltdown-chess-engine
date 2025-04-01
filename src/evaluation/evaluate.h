#pragma once

#include "bit_board.h"
#include "evaluation/evaluation_weights.h"
#include "evaluation/position_tables.h"
#include "helpers/bit_operations.h"
#include "movegen/bishops.h"
#include "movegen/kings.h"
#include "movegen/knights.h"
#include "movegen/rooks.h"

#include <array>

#ifdef TUNING
#include "src/tracer.h"
namespace evaluation {
static inline EvaluationTrace s_trace {};

#define INC_SCORE(weightName)          \
    {                                  \
        score += s_weights.weightName; \
        s_trace.weightName[player]++;  \
    }

#define INC_SCORE_INDEXED(weightName, index)  \
    {                                         \
        score += s_weights.weightName[index]; \
        s_trace.weightName[index][player]++;  \
    }

}
#else

#define INC_SCORE(weightName)          \
    {                                  \
        score += s_weights.weightName; \
    }

#define INC_SCORE_INDEXED(weightName, index)  \
    {                                         \
        score += s_weights.weightName[index]; \
    }

#endif

namespace evaluation {

[[nodiscard]] constexpr uint64_t flipPosition(BoardPosition pos) noexcept
{
    return pos ^ 56;
}

constexpr std::array<uint8_t, magic_enum::enum_count<Piece>()> s_materialPhaseScore = {
    0, 1, 1, 2, 4, 0, /* pieces */
};

template<Player player>
constexpr static inline Score getPawnScore(const BitBoard& board, const uint64_t pawns)
{
    Score score(0, 0);

    helper::bitIterate(pawns, [&](BoardPosition pos) {
        INC_SCORE_INDEXED(pieceValues, Pawn);

        const auto doubledPawns = std::popcount(pawns & s_fileMaskTable[pos]);
        if (doubledPawns > 1)
            INC_SCORE(doublePawnPenalty);

        if ((pawns & s_isolationMaskTable[pos]) == 0)
            INC_SCORE(isolatedPawnPenalty);

        if constexpr (player == PlayerWhite) {
            INC_SCORE_INDEXED(psqtPawns, pos);

            if ((board.pieces[BlackPawn] & s_whitePassedPawnMaskTable[pos]) == 0) {
                const uint8_t row = (pos / 8);
                INC_SCORE_INDEXED(passedPawnBonus, row);
            }
        } else {
            INC_SCORE_INDEXED(psqtPawns, flipPosition(pos));

            if ((board.pieces[WhitePawn] & s_blackPassedPawnMaskTable[pos]) == 0) {
                const uint8_t row = 7 - (pos / 8);
                INC_SCORE_INDEXED(passedPawnBonus, row);
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
        phaseScore += s_materialPhaseScore[Knight];
        INC_SCORE_INDEXED(pieceValues, Knight);

        const uint64_t moves = movegen::getKnightMoves(pos) & ~board.occupation[player];
        const int movesCount = std::popcount(moves);
        INC_SCORE_INDEXED(knightMobilityScore, movesCount);

        if constexpr (player == PlayerWhite) {
            INC_SCORE_INDEXED(psqtKnights, pos);
        } else {
            INC_SCORE_INDEXED(psqtKnights, flipPosition(pos));
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
        INC_SCORE(bishopPairScore)

    helper::bitIterate(bishops, [&](BoardPosition pos) {
        phaseScore += s_materialPhaseScore[Bishop];
        INC_SCORE_INDEXED(pieceValues, Bishop);

        const uint64_t moves = movegen::getBishopMoves(pos, board.occupation[Both]);
        const int movesCount = std::popcount(moves);
        INC_SCORE_INDEXED(bishopMobilityScore, movesCount);

        if constexpr (player == PlayerWhite) {
            INC_SCORE_INDEXED(psqtBishops, pos);
        } else {
            INC_SCORE_INDEXED(psqtBishops, flipPosition(pos));
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
        phaseScore += s_materialPhaseScore[Rook];
        INC_SCORE_INDEXED(pieceValues, Rook);

        const uint64_t moves = movegen::getRookMoves(pos, board.occupation[Both]);
        const int movesCount = std::popcount(moves);
        INC_SCORE_INDEXED(rookMobilityScore, movesCount);

        if (((whitePawns | blackPawns) & s_fileMaskTable[pos]) == 0)
            INC_SCORE(rookOpenFileBonus);

        if constexpr (player == PlayerWhite) {
            INC_SCORE_INDEXED(psqtRooks, pos);
            if ((whitePawns & s_fileMaskTable[pos]) == 0)
                INC_SCORE(rookSemiOpenFileBonus);
        } else {
            INC_SCORE_INDEXED(psqtRooks, flipPosition(pos));
            if ((blackPawns & s_fileMaskTable[pos]) == 0)
                INC_SCORE(rookSemiOpenFileBonus);
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
        phaseScore += s_materialPhaseScore[Queen];
        INC_SCORE_INDEXED(pieceValues, Queen);

        if (((whitePawns | blackPawns) & s_fileMaskTable[pos]) == 0)
            INC_SCORE(queenOpenFileBonus);

        const uint64_t moves
            = movegen::getBishopMoves(pos, board.occupation[Both])
            | movegen::getRookMoves(pos, board.occupation[Both]);
        const int movesCount = std::popcount(moves);
        INC_SCORE_INDEXED(queenMobilityScore, movesCount);

        if constexpr (player == PlayerWhite) {
            INC_SCORE_INDEXED(psqtQueens, pos);
            if ((whitePawns & s_fileMaskTable[pos]) == 0)
                INC_SCORE(queenSemiOpenFileBonus);
        } else {
            INC_SCORE_INDEXED(psqtQueens, flipPosition(pos));
            if ((blackPawns & s_fileMaskTable[pos]) == 0)
                INC_SCORE(queenSemiOpenFileBonus);
        }
    });

    return score;
}

template<Player player>
constexpr static inline Score getKingScore(const BitBoard& board, const uint64_t king)
{
    Score score(0, 0);

    helper::bitIterate(king, [&](BoardPosition pos) {
        const uint64_t kingShields = movegen::getKingMoves(pos) & board.occupation[player];
        const int shieldCount = std::popcount(kingShields);
        INC_SCORE_INDEXED(kingShieldBonus, shieldCount);

        if constexpr (player == PlayerWhite) {
            INC_SCORE_INDEXED(psqtKings, pos);

        } else {
            INC_SCORE_INDEXED(psqtKings, flipPosition(pos));
        }
    });

    return score;
}

}

