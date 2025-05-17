#pragma once

#include "attack_generation.h"
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

[[nodiscard]] constexpr uint64_t flipPosition(BoardPosition pos) noexcept
{
    return pos ^ 56;
}

constexpr uint8_t pawnShieldSize = s_terms.pawnShieldBonus.size();
constexpr uint8_t majorsOn7thSize = s_terms.majorOn7thScore.size();

/* TODO: divide into terms instead of pieces.. */

template<Player player>
constexpr static inline TermScore getPawnScore(const BitBoard& board, const uint64_t pawns)
{
    constexpr Piece ourKing = player == PlayerWhite ? WhiteKing : BlackKing;
    TermScore score(0, 0);

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
constexpr static inline TermScore getKnightScore(const BitBoard& board, const uint64_t knights, uint8_t& phaseScore)
{
    TermScore score(0, 0);

    const uint64_t whitePawns = board.pieces[WhitePawn];
    const uint64_t blackPawns = board.pieces[BlackPawn];

    const uint64_t pawnDefends = player == PlayerWhite ? attackgen::getWhitePawnAttacks(board) : attackgen::getBlackPawnAttacks(board);

    helper::bitIterate(knights, [&](BoardPosition pos) {
        phaseScore += s_piecePhaseValues[Knight];
        ADD_SCORE_INDEXED(pieceValues, Knight);

        const uint64_t moves = movegen::getKnightMoves(pos) & ~board.occupation[player];
        const int movesCount = std::popcount(moves);
        ADD_SCORE_INDEXED(knightMobilityScore, movesCount);

        const uint64_t square = helper::positionToSquare(pos);

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
constexpr static inline TermScore getBishopScore(const BitBoard& board, const uint64_t bishops, uint8_t& phaseScore)
{
    TermScore score(0, 0);

    const uint64_t whitePawns = board.pieces[WhitePawn];
    const uint64_t blackPawns = board.pieces[BlackPawn];

    const uint64_t pawnDefends = player == PlayerWhite ? attackgen::getWhitePawnAttacks(board) : attackgen::getBlackPawnAttacks(board);

    const int amntBishops = std::popcount(bishops);
    if (amntBishops >= 2)
        ADD_SCORE(bishopPairScore)

    helper::bitIterate(bishops, [&](BoardPosition pos) {
        phaseScore += s_piecePhaseValues[Bishop];
        ADD_SCORE_INDEXED(pieceValues, Bishop);

        const uint64_t moves = movegen::getBishopMoves(pos, board.occupation[Both]);
        const int movesCount = std::popcount(moves);
        ADD_SCORE_INDEXED(bishopMobilityScore, movesCount);

        const uint64_t square = helper::positionToSquare(pos);

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
constexpr static inline TermScore getRookScore(const BitBoard& board, const uint64_t rooks, uint8_t& phaseScore)
{
    TermScore score(0, 0);

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
constexpr static inline TermScore getQueenScore(const BitBoard& board, const uint64_t queens, uint8_t& phaseScore)
{
    TermScore score(0, 0);

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
constexpr static inline TermScore getKingScore(const BitBoard& board, const uint64_t king)
{
    TermScore score(0, 0);

    helper::bitIterate(king, [&](BoardPosition pos) {
        /* virtual mobility - replace king with queen to see potential attacks for sliding pieces */
        const uint64_t virtualMoves
            = movegen::getBishopMoves(pos, board.occupation[Both])
            | movegen::getRookMoves(pos, board.occupation[Both]);
        const int movesCount = std::popcount(virtualMoves);
        ADD_SCORE_INDEXED(kingVirtualMobilityScore, movesCount);

        if constexpr (player == PlayerWhite) {
            ADD_SCORE_INDEXED(psqtKings, pos);
        } else {
            ADD_SCORE_INDEXED(psqtKings, flipPosition(pos));
        }
    });

    return score;
}

template<Player player>
constexpr static inline TermScore getMajorsOn7thScore(const BitBoard& board)
{
    TermScore score(0, 0);

    constexpr auto ownQueen = player == PlayerWhite ? WhiteQueen : BlackQueen;
    constexpr auto ownRook = player == PlayerWhite ? WhiteRook : BlackRook;

    constexpr auto theirKing = player == PlayerWhite ? BlackKing : WhiteKing;
    constexpr auto theirPawn = player == PlayerWhite ? BlackPawn : WhitePawn;

    constexpr uint64_t row7Mask = player == PlayerWhite ? s_row7Mask : s_row2Mask;
    constexpr uint64_t row8Mask = player == PlayerWhite ? s_row8Mask : s_row1Mask;

    const uint64_t pawnsOn7th = board.pieces[theirPawn] & row7Mask;
    const uint64_t kingOn8th = board.pieces[theirKing] & row8Mask;
    const uint64_t majorsOn7th = (board.pieces[ownQueen] | board.pieces[ownRook]) & row7Mask;
    const uint8_t majorsOn7thCount = std::min<uint8_t>(majorsOn7thSize, std::popcount(majorsOn7th));

    if (majorsOn7thCount > 0 && (pawnsOn7th || kingOn8th)) {
        ADD_SCORE_INDEXED(majorOn7thScore, majorsOn7thCount - 1);
    }

    return score;
}

}
