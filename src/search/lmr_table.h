#pragma once

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace search {

namespace {

/* values borrowed from Carp */
constexpr size_t lmrTableSize { 64 };
constexpr double lmrBase { 0.75 };
constexpr double lmrFactor { 2.25 };

using LmrMoveCountEntry = std::array<uint8_t, lmrTableSize>;

inline auto createLmrTable()
{
    std::array<LmrMoveCountEntry, lmrTableSize> table {};

    for (uint8_t depth = 0; depth < lmrTableSize; depth++) {
        for (uint8_t moveCount = 0; moveCount < lmrTableSize; moveCount++) {
            table[depth][moveCount] = static_cast<uint8_t>(std::log(lmrBase + depth) * std::log(moveCount) / lmrFactor);
        }
    }

    return table;
}

const static inline auto s_lmrTable = createLmrTable();

}

static inline uint8_t getLmrReduction(uint8_t depth, uint8_t moveCount)
{
    constexpr uint8_t minVal = lmrTableSize - 1;

    return s_lmrTable[std::min(depth, minVal)][std::min(moveCount, minVal)];
}

}
