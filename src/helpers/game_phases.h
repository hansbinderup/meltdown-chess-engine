#pragma once

#include "evaluation/game_phase.h"
#include "evaluation/pesto_tables.h"
#include "magic_enum/magic_enum.hpp"

namespace helper {

/* Utility to create a std::array<T, N> for phase-based evaluation constants.
 * Ensures the number of provided values matches the number of game phases.
 * T must be explicitly specified when using complex types like nested arrays.
 * Example: createPhaseArray<int32_t>(10, 10)
 *         createPhaseArray<std::array<int32_t, 8>>(row1, row2)
 * NOTE: We require T to be explicit to avoid deduction issues with brace-initialized types. */
template<typename T, typename... Args>
constexpr auto createPhaseArray(Args&&... values)
    -> std::array<T, sizeof...(Args)>
{
    static_assert(sizeof...(Args) == magic_enum::enum_count<evaluation::gamephase::Phases>(),
        "Wrong number of phase values");
    return std::array<T, sizeof...(Args)> { std::forward<Args>(values)... };
}

/* Adds a phase-specific score (midgame and endgame) to the given score object.
 * `values` must be an array of size 2, where index 0 = midgame, index 1 = endgame.
 */
template<typename T>
constexpr void addPhaseScore(evaluation::gamephase::Score& score, const std::array<T, 2>& values)
{
    score.mg += values[evaluation::gamephase::GamePhaseMg];
    score.eg += values[evaluation::gamephase::GamePhaseEg];
}

/* Adds a phase-specific score to the given score object, scaled by a float multiplier.
 * Useful when applying scores based on piece counts, shield counts, mobility, etc.
 */
template<typename T>
constexpr void addPhaseScore(evaluation::gamephase::Score& score, const std::array<T, 2>& values, float multiplier)
{
    score.mg += values[evaluation::gamephase::GamePhaseMg] * multiplier;
    score.eg += values[evaluation::gamephase::GamePhaseEg] * multiplier;
}

/* Subtracts a phase-specific score from the given score object.
 * Mirrors addPhaseScore(), but for penalties or score reductions.
 */
template<typename T>
constexpr void subPhaseScore(evaluation::gamephase::Score& score, const std::array<T, 2>& values)
{
    score.mg -= values[evaluation::gamephase::GamePhaseMg];
    score.eg -= values[evaluation::gamephase::GamePhaseEg];
}

/* Subtracts a phase-specific score from the score object, scaled by a float multiplier.
 * Useful when reducing score based on factors like lack of development, pawn weaknesses, etc.
 */
template<typename T>
constexpr void subPhaseScore(evaluation::gamephase::Score& score, const std::array<T, 2>& values, float multiplier)
{
    score.mg -= values[evaluation::gamephase::GamePhaseMg] * multiplier;
    score.eg -= values[evaluation::gamephase::GamePhaseEg] * multiplier;
}

/* Adds midgame and endgame scores for a specific piece at a given board position,
 * using precomputed PESTO piece-square tables.
 * Also the material score (game phase dependent) is increased whenever probed
 */
constexpr void addPestoPhaseScore(evaluation::gamephase::Score& score, Piece piece, BoardPosition pos)
{
    score.mg += evaluation::pesto::s_mgTables[piece][pos];
    score.eg += evaluation::pesto::s_egTables[piece][pos];
    score.materialScore += evaluation::gamephase::s_materialPhaseScore[piece];
}

}

