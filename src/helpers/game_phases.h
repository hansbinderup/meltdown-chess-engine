#pragma once

#include "evaluation/game_phase.h"
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

}

