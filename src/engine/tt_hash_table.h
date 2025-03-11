#pragma once

#include "board_defs.h"
#include "movegen/move_types.h"
#include <algorithm>
#include <array>
#include <cstdint>

namespace engine {

enum TtHashFlag : uint8_t {
    TtHashExact = 0,
    TtHashAlpha = 1,
    TtHashBeta = 2,
};

struct TtHashEntry {
    uint64_t key = 0;
    uint8_t depth = 0;
    TtHashFlag flag = TtHashExact;
    int32_t score = 0;
    uint8_t generation = 0;
    movegen::Move move {};
};

class TtHashTable {
public:
    static void clear()
    {
        std::ranges::fill(s_ttHashTable, TtHashEntry {});
        s_hashCount = 0;
        s_currentGeneration = 0;
    }

    static void advanceGeneration()
    {
        s_currentGeneration = (s_currentGeneration + 1) & s_generationMask; /* Increment and wrap - max gen */
    }

    static uint16_t getHashFull()
    {
        /* should be returned as permill */
        return std::min((1000 * s_hashCount) / s_ttHashSize, 1000UL);
    }

    using ProbeResult = std::pair<int32_t, movegen::Move>;
    constexpr static std::optional<ProbeResult> probe(uint64_t key, int32_t alpha, int32_t beta, uint8_t depth, uint8_t ply)
    {
        auto& entry = s_ttHashTable[key % s_ttHashSize];

        if (entry.key != key) {
            return std::nullopt;
        }

        /* Entry is outdated (from a previous generation) */
        if (entry.generation != s_currentGeneration) {
            entry.key = 0; // Invalidate the entry
            if (s_hashCount > 0) {
                --s_hashCount;
            }
            return std::nullopt;
        }

        if (entry.depth >= depth) {
            int32_t score = entry.score;

            /* special case when mating score is found */
            if (score < -s_mateScore)
                score += ply;
            if (score > s_mateScore)
                score -= ply;

            if (entry.flag == TtHashExact) {
                return std::make_pair(score, entry.move);
            }

            if ((entry.flag == TtHashAlpha) && (score <= alpha)) {
                return std::make_pair(alpha, entry.move);
            }

            if ((entry.flag == TtHashBeta) && (score >= beta)) {
                return std::make_pair(beta, entry.move);
            }
        }

        return std::nullopt;
    }

    constexpr static void writeEntry(uint64_t key, int32_t score, const movegen::Move& move, uint8_t depth, uint8_t ply, TtHashFlag flag)
    {
        auto& entry = s_ttHashTable[key % s_ttHashSize];

        /* special case when mating score is found */
        if (score < -s_mateScore)
            score -= ply;
        if (score > s_mateScore)
            score += ply;

        if (entry.key == 0) {
            ++s_hashCount;
        }

        entry.key = key;
        entry.score = score;
        entry.flag = flag;
        entry.depth = depth;
        entry.generation = s_currentGeneration;
        entry.move = move;
    }

private:
    constexpr static inline std::size_t s_hashTableSizeMb { 128 };
    constexpr static inline std::size_t s_ttHashSize { (s_hashTableSizeMb * 1024 * 1024) / sizeof(TtHashEntry) };
    constexpr static inline uint8_t s_generationMask { 0xF }; /* 4-bit wrapping generation (max 16 generations allowed) */

    static inline std::array<TtHashEntry, s_ttHashSize> s_ttHashTable; /* TODO: make dynamic so size can be adjusted from UCI */
    static inline uint64_t s_hashCount {};
    static inline uint8_t s_currentGeneration {};
};
}

