#pragma once

#include "bit_board.h"
#include "board_defs.h"
#include <array>
#include <cstdint>

#include "evaluation/game_phase.h"
#include "evaluation/pesto_tables.h"
#include "evaluation/position_tables.h"
#include "helpers/bit_operations.h"
#include "movegen/bishops.h"
#include "movegen/kings.h"
#include "movegen/rooks.h"

namespace evaluation {

namespace {

/* TODO: tune these parameters */
constexpr std::array<int32_t, magic_enum::enum_count<gamephase::Phases>()> s_doublePawnPenalty { -10, -10 };
constexpr std::array<int32_t, magic_enum::enum_count<gamephase::Phases>()> s_isolatedPawnPenalty { -10, -10 };

using PassedPawnType = std::array<int32_t, 8>;
constexpr std::array<PassedPawnType, magic_enum::enum_count<gamephase::Phases>()> s_passedPawnBonus { {
    { 0, 10, 30, 50, 75, 100, 150, 200 },
    { 0, 10, 30, 50, 75, 100, 150, 200 },
} };

constexpr std::array<int32_t, magic_enum::enum_count<gamephase::Phases>()> s_semiOpenFileScore { 15, 15 };
constexpr std::array<int32_t, magic_enum::enum_count<gamephase::Phases>()> s_openFileScore { 15, 15 };

constexpr std::array<int32_t, magic_enum::enum_count<gamephase::Phases>()> s_bishopMobilityScore { 3, 3 };
constexpr std::array<int32_t, magic_enum::enum_count<gamephase::Phases>()> s_queenMobilityScore { 1, 1 };
constexpr std::array<int32_t, magic_enum::enum_count<gamephase::Phases>()> s_kingShieldScore { 5, 5 };

}

namespace position {

template<Player player>
constexpr static inline gamephase::Score getPawnScore(const BitBoard& board, const uint64_t pawns)
{
    gamephase::Score score;

    helper::bitIterate(pawns, [&](BoardPosition pos) {
        const auto doubledPawns = std::popcount(pawns & s_fileMaskTable[pos]);
        if (doubledPawns > 1) {
            score.mg += s_doublePawnPenalty[gamephase::GamePhaseMg];
            score.eg += s_doublePawnPenalty[gamephase::GamePhaseEg];
        }

        if ((pawns & s_isolationMaskTable[pos]) == 0) {
            score.mg += s_isolatedPawnPenalty[gamephase::GamePhaseMg];
            score.eg += s_isolatedPawnPenalty[gamephase::GamePhaseEg];
        }

        if constexpr (player == PlayerWhite) {
            score.materialScore += gamephase::s_materialPhaseScore[WhitePawn];
            score.mg += pesto::s_mgTables[WhitePawn][pos];
            score.eg += pesto::s_egTables[WhitePawn][pos];

            if ((board.pieces[BlackPawn] & s_whitePassedPawnMaskTable[pos]) == 0) {
                const uint8_t row = (pos / 8);
                score.mg += s_passedPawnBonus[gamephase::GamePhaseMg][row];
                score.eg += s_passedPawnBonus[gamephase::GamePhaseEg][row];
            }
        } else {
            score.materialScore += gamephase::s_materialPhaseScore[BlackPawn];
            score.mg += pesto::s_mgTables[BlackPawn][pos];
            score.eg += pesto::s_egTables[BlackPawn][pos];

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
        if constexpr (player == PlayerWhite) {
            score.materialScore += gamephase::s_materialPhaseScore[WhiteKnight];
            score.mg += pesto::s_mgTables[WhiteKnight][pos];
            score.eg += pesto::s_egTables[WhiteKnight][pos];
        } else {
            score.materialScore += gamephase::s_materialPhaseScore[BlackKnight];
            score.mg += pesto::s_mgTables[BlackKnight][pos];
            score.eg += pesto::s_egTables[BlackKnight][pos];
        }
    });

    return score;
}

template<Player player>
constexpr static inline gamephase::Score getBishopScore(const BitBoard& board, const uint64_t bishops)
{
    gamephase::Score score;

    helper::bitIterate(bishops, [&score, &board](BoardPosition pos) {
        const uint64_t moves = movegen::getBishopMoves(pos, board.occupation[Both]);
        const int count = std::popcount(moves);

        score.mg += count * s_bishopMobilityScore[gamephase::GamePhaseMg];
        score.eg += count * s_bishopMobilityScore[gamephase::GamePhaseEg];

        if constexpr (player == PlayerWhite) {
            score.materialScore += gamephase::s_materialPhaseScore[WhiteBishop];
            score.mg += pesto::s_mgTables[WhiteBishop][pos];
            score.eg += pesto::s_egTables[WhiteBishop][pos];
        } else {
            score.materialScore += gamephase::s_materialPhaseScore[BlackBishop];
            score.mg += pesto::s_mgTables[BlackBishop][pos];
            score.eg += pesto::s_egTables[BlackBishop][pos];
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
        if (((whitePawns | blackPawns) & s_fileMaskTable[pos]) == 0) {
            score.mg += s_openFileScore[gamephase::GamePhaseMg];
            score.eg += s_openFileScore[gamephase::GamePhaseEg];
        }

        if constexpr (player == PlayerWhite) {
            score.materialScore += gamephase::s_materialPhaseScore[WhiteRook];
            score.mg += pesto::s_mgTables[WhiteRook][pos];
            score.eg += pesto::s_egTables[WhiteRook][pos];

            if ((whitePawns & s_fileMaskTable[pos]) == 0) {
                score.mg += s_semiOpenFileScore[gamephase::GamePhaseMg];
                score.eg += s_semiOpenFileScore[gamephase::GamePhaseEg];
            }
        } else {
            score.materialScore += gamephase::s_materialPhaseScore[BlackRook];
            score.mg += pesto::s_mgTables[BlackRook][pos];
            score.eg += pesto::s_egTables[BlackRook][pos];

            if ((blackPawns & s_fileMaskTable[pos]) == 0) {
                score.mg += s_semiOpenFileScore[gamephase::GamePhaseMg];
                score.eg += s_semiOpenFileScore[gamephase::GamePhaseEg];
            }
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

        const int count = std::popcount(moves);

        score.mg += count * s_queenMobilityScore[gamephase::GamePhaseMg];
        score.eg += count * s_queenMobilityScore[gamephase::GamePhaseEg];

        if constexpr (player == PlayerWhite) {
            score.materialScore += gamephase::s_materialPhaseScore[WhiteQueen];
            score.mg += pesto::s_mgTables[WhiteQueen][pos];
            score.eg += pesto::s_egTables[WhiteQueen][pos];
        } else {
            score.materialScore += gamephase::s_materialPhaseScore[BlackQueen];
            score.mg += pesto::s_mgTables[BlackQueen][pos];
            score.eg += pesto::s_egTables[BlackQueen][pos];
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
        if (((whitePawns | blackPawns) & s_fileMaskTable[pos]) == 0) {
            score.mg -= s_openFileScore[gamephase::GamePhaseMg];
            score.eg -= s_openFileScore[gamephase::GamePhaseEg];
        }

        if constexpr (player == PlayerWhite) {
            score.mg += pesto::s_mgTables[WhiteKing][pos];
            score.eg += pesto::s_egTables[WhiteKing][pos];

            if ((whitePawns & s_fileMaskTable[pos]) == 0) {
                score.mg -= s_semiOpenFileScore[gamephase::GamePhaseMg];
                score.eg -= s_semiOpenFileScore[gamephase::GamePhaseEg];
            }

            const uint64_t kingShields = movegen::getKingMoves(pos) & board.occupation[PlayerWhite];
            const int shieldCount = std::popcount(kingShields);
            score.mg += shieldCount * s_kingShieldScore[gamephase::GamePhaseMg];
            score.eg += shieldCount * s_kingShieldScore[gamephase::GamePhaseEg];
        } else {
            score.mg += pesto::s_mgTables[BlackKing][pos];
            score.eg += pesto::s_egTables[BlackKing][pos];

            if ((blackPawns & s_fileMaskTable[pos]) == 0) {
                score.mg -= s_semiOpenFileScore[gamephase::GamePhaseMg];
                score.eg -= s_semiOpenFileScore[gamephase::GamePhaseEg];
            }

            const uint64_t kingShields = movegen::getKingMoves(pos) & board.occupation[PlayerBlack];
            const int shieldCount = std::popcount(kingShields);
            score.mg += shieldCount * s_kingShieldScore[gamephase::GamePhaseMg];
            score.eg += shieldCount * s_kingShieldScore[gamephase::GamePhaseEg];
        }
    });

    return score;
}

}

}
