#pragma once

#include "bit_board.h"
#include "board_defs.h"
#include <array>
#include <cstdint>

#include "evaluation/game_phase.h"
#include "evaluation/position_tables.h"
#include "helpers/bit_operations.h"
#include "helpers/game_phases.h"
#include "movegen/bishops.h"
#include "movegen/kings.h"
#include "movegen/knights.h"
#include "movegen/rooks.h"

namespace evaluation {

namespace {

/* TODO: find a way to tune these parameters in an automated way - currently they're just guesses
 * NOTE: All values are duplicates as no EG tuning has been performed yet */

constexpr auto s_doublePawnPenalty = helper::createPhaseArray<int32_t>(-10, -10);
constexpr auto s_isolatedPawnPenalty = helper::createPhaseArray<int32_t>(-10, -10);

using PassedPawnType = std::array<int32_t, 8>;
constexpr auto s_passedPawnBonus = helper::createPhaseArray<PassedPawnType>(
    PassedPawnType { 0, 10, 30, 50, 75, 100, 150, 200 },
    PassedPawnType { 0, 10, 30, 50, 75, 100, 150, 200 });

constexpr auto s_openFileScore = helper::createPhaseArray<int32_t>(15, 15);
constexpr auto s_semiOpenFileScore = helper::createPhaseArray<int32_t>(10, 10);

constexpr auto s_bishopMobilityScore = helper::createPhaseArray<int32_t>(3, 3);
constexpr auto s_bishopPairScore = helper::createPhaseArray<int32_t>(0, 0);

constexpr auto s_knightMobilityScore = helper::createPhaseArray<int32_t>(0, 0);
constexpr auto s_rookMobilityScore = helper::createPhaseArray<int32_t>(0, 0);
constexpr auto s_queenMobilityScore = helper::createPhaseArray<int32_t>(1, 1);
constexpr auto s_kingShieldScore = helper::createPhaseArray<int32_t>(5, 5);

}

namespace position {

template<Player player>
constexpr static inline gamephase::Score getPawnScore(const BitBoard& board, const uint64_t pawns)
{
    gamephase::Score score;

    helper::bitIterate(pawns, [&](BoardPosition pos) {
        const auto doubledPawns = std::popcount(pawns & s_fileMaskTable[pos]);
        if (doubledPawns > 1)
            helper::addPhaseScore(score, s_doublePawnPenalty);

        if ((pawns & s_isolationMaskTable[pos]) == 0)
            helper::addPhaseScore(score, s_isolatedPawnPenalty);

        if constexpr (player == PlayerWhite) {
            helper::addPestoPhaseScore(score, WhitePawn, pos);

            if ((board.pieces[BlackPawn] & s_whitePassedPawnMaskTable[pos]) == 0) {
                const uint8_t row = (pos / 8);
                score.mg += s_passedPawnBonus[gamephase::GamePhaseMg][row];
                score.eg += s_passedPawnBonus[gamephase::GamePhaseEg][row];
            }
        } else {
            helper::addPestoPhaseScore(score, BlackPawn, pos);

            if ((board.pieces[WhitePawn] & s_blackPassedPawnMaskTable[pos]) == 0) {
                const uint8_t row = (pos / 8);
                score.mg += s_passedPawnBonus[gamephase::GamePhaseMg][7 - row];
                score.eg += s_passedPawnBonus[gamephase::GamePhaseEg][7 - row];
            }
        }
    });

    return score;
}

template<Player player>
constexpr static inline gamephase::Score getKnightScore(const uint64_t knights)
{
    gamephase::Score score;

    helper::bitIterate(knights, [&score](BoardPosition pos) {
        const uint64_t moves = movegen::getKnightMoves(pos);
        const int movesCount = std::popcount(moves);
        helper::addPhaseScore(score, s_knightMobilityScore, movesCount);

        if constexpr (player == PlayerWhite) {
            helper::addPestoPhaseScore(score, WhiteKnight, pos);
        } else {
            helper::addPestoPhaseScore(score, BlackKnight, pos);
        }
    });

    return score;
}

template<Player player>
constexpr static inline gamephase::Score getBishopScore(const BitBoard& board, const uint64_t bishops)
{
    gamephase::Score score;

    const int amntBishops = std::popcount(bishops);
    if (amntBishops >= 2)
        helper::addPhaseScore(score, s_bishopPairScore);

    helper::bitIterate(bishops, [&score, &board](BoardPosition pos) {
        const uint64_t moves = movegen::getBishopMoves(pos, board.occupation[Both]);
        const int movesCount = std::popcount(moves);
        helper::addPhaseScore(score, s_bishopMobilityScore, movesCount);

        if constexpr (player == PlayerWhite) {
            helper::addPestoPhaseScore(score, WhiteBishop, pos);
        } else {
            helper::addPestoPhaseScore(score, BlackBishop, pos);
        }
    });

    return score;
}

template<Player player>
constexpr gamephase::Score getRookScore(const BitBoard& board, const uint64_t rooks)
{
    gamephase::Score score;

    const uint64_t whitePawns = board.pieces[WhitePawn];
    const uint64_t blackPawns = board.pieces[BlackPawn];

    helper::bitIterate(rooks, [&](BoardPosition pos) {
        const uint64_t moves = movegen::getRookMoves(pos, board.occupation[Both]);
        const int movesCount = std::popcount(moves);
        helper::addPhaseScore(score, s_rookMobilityScore, movesCount);

        if (((whitePawns | blackPawns) & s_fileMaskTable[pos]) == 0)
            helper::addPhaseScore(score, s_openFileScore);

        if constexpr (player == PlayerWhite) {
            helper::addPestoPhaseScore(score, WhiteRook, pos);

            if ((whitePawns & s_fileMaskTable[pos]) == 0)
                helper::addPhaseScore(score, s_semiOpenFileScore);
        } else {
            helper::addPestoPhaseScore(score, BlackRook, pos);

            if ((blackPawns & s_fileMaskTable[pos]) == 0)
                helper::addPhaseScore(score, s_semiOpenFileScore);
        }
    });

    return score;
}

template<Player player>
constexpr static inline gamephase::Score getQueenScore(const BitBoard& board, const uint64_t queens)
{
    gamephase::Score score;

    helper::bitIterate(queens, [&board, &score](BoardPosition pos) {
        const uint64_t moves
            = movegen::getBishopMoves(pos, board.occupation[Both])
            | movegen::getRookMoves(pos, board.occupation[Both]);

        const int movesCount = std::popcount(moves);
        helper::addPhaseScore(score, s_queenMobilityScore, movesCount);

        if constexpr (player == PlayerWhite) {
            helper::addPestoPhaseScore(score, WhiteQueen, pos);
        } else {
            helper::addPestoPhaseScore(score, BlackQueen, pos);
        }
    });

    return score;
}

template<Player player>
constexpr static inline gamephase::Score getKingScore(const BitBoard& board, const uint64_t king)
{
    gamephase::Score score;

    const uint64_t whitePawns = board.pieces[WhitePawn];
    const uint64_t blackPawns = board.pieces[BlackPawn];

    helper::bitIterate(king, [&](BoardPosition pos) {
        /* king will get a penalty for semi/open files */
        if (((whitePawns | blackPawns) & s_fileMaskTable[pos]) == 0)
            helper::subPhaseScore(score, s_openFileScore);

        if constexpr (player == PlayerWhite) {
            helper::addPestoPhaseScore(score, WhiteKing, pos);

            if ((whitePawns & s_fileMaskTable[pos]) == 0)
                helper::subPhaseScore(score, s_semiOpenFileScore);

            const uint64_t kingShields = movegen::getKingMoves(pos) & board.occupation[PlayerWhite];
            const int shieldCount = std::popcount(kingShields);
            helper::addPhaseScore(score, s_kingShieldScore, shieldCount);
        } else {
            helper::addPestoPhaseScore(score, BlackKing, pos);

            if ((blackPawns & s_fileMaskTable[pos]) == 0)
                helper::subPhaseScore(score, s_semiOpenFileScore);

            const uint64_t kingShields = movegen::getKingMoves(pos) & board.occupation[PlayerBlack];
            const int shieldCount = std::popcount(kingShields);
            helper::addPhaseScore(score, s_kingShieldScore, shieldCount);
        }
    });

    return score;
}

}

}
