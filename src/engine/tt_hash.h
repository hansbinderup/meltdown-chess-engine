#pragma once

#include <algorithm>
#include <array>
#include <cstdint>

namespace engine::tt {

namespace {

constexpr std::size_t s_ttHashSize { 0x1600000 };

enum TtHashFlag : uint8_t {
    TtHashExact = 0,
    TtHashAlpha = 1,
    TtHashBeta = 2,
};

struct TtHashEntry {
    uint64_t key = 0;
    uint8_t depth = 0;
    TtHashFlag flag = TtHashExact;
    int16_t score = 0;
};

static inline std::array<TtHashEntry, s_ttHashSize> s_ttHashTable;

}

// clear TT (hash table)
inline void clearTable()
{
    std::ranges::fill(s_ttHashTable, TtHashEntry {});
}

constexpr inline std::optional<int16_t> readHashEntry(uint64_t key, int16_t alpha, int16_t beta, uint8_t depth)
{
    auto& entry = s_ttHashTable[key % s_ttHashSize];

    if (entry.key != key) {
        return std::nullopt;
    }

    if (entry.depth >= depth) {
        if (entry.flag == TtHashExact) {
            return entry.score;
        }

        if ((entry.flag == TtHashAlpha) && (entry.score <= alpha)) {
            return alpha;
        }

        if ((entry.flag == TtHashBeta) && (entry.score >= beta)) {
            return beta;
        }
    }

    return std::nullopt;
}

constexpr inline void writeHashEntry(uint64_t key, int16_t score, uint8_t depth, TtHashFlag flag)
{
    auto& entry = s_ttHashTable[key % s_ttHashSize];

    entry.key = key;
    entry.score = score;
    entry.flag = flag;
    entry.depth = depth;
}

}

