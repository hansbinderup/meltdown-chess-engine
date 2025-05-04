#pragma once

#include "board_defs.h"
#include "fmt/base.h"
#include "helpers/memory.h"
#include "movegen/move_types.h"
#include <algorithm>
#include <atomic>
#include <cassert>
#include <cstdint>

namespace engine {

enum TtHashFlag : uint8_t {
    TtHashExact = 0,
    TtHashAlpha = 1,
    TtHashBeta = 2,
};

struct TtHashEntry {
    uint64_t key = 0;
    TtHashFlag flag;
    Score score {};
    movegen::Move move {};
    uint8_t depth {};
};

class TtHashTable {
public:
    /* frees the memory of the current table (if any) and allocates the amount provided in MB
     * NOTE: NOT THREAD SAFE */
    static void setSizeMb(std::size_t sizeMb)
    {
        if (sizeMb <= 0) {
            fmt::println("Invalid size: {}mb", sizeMb);
            return;
        }

        if (s_ttHashSize > 0) {
            helper::alignedFree(s_ttHashTable);
        }

        s_ttHashSize = tableSizeFromMb(sizeMb);
        s_ttHashTable = static_cast<TtHashEntry*>(helper::alignedAlloc(alignof(TtHashEntry), s_ttHashSize * sizeof(TtHashEntry)));

        if (s_ttHashTable == nullptr) {
            fmt::println("Could not allocate memory for hash table");
            s_ttHashSize = 0;
        }

        clear();
    }

    static std::size_t getSizeMb()
    {
        return (s_ttHashSize / 1024 / 1024) * sizeof(TtHashEntry);
    }

    static void clear()
    {
        for (size_t i = 0; i < s_ttHashSize; i++) {
            s_ttHashTable[i] = TtHashEntry {};
        }

        s_hashCount = 0;
    }

    static uint16_t getHashFull()
    {
        /* should be returned as permill */
        constexpr uint16_t maxValue { 1000 };
        const uint16_t permill = (1000 * s_hashCount.load(std::memory_order_relaxed)) / s_ttHashSize;

        return std::min(permill, maxValue);
    }

    struct ProbeResult {
        Score score;
        TtHashFlag flag;
        movegen::Move move;
    };

    constexpr static std::optional<ProbeResult> probe(uint64_t key, uint8_t depth, uint8_t ply)
    {
        assert(s_ttHashSize > 0);

        auto& entry = s_ttHashTable[key % s_ttHashSize];

        if (entry.key != key) {
            return std::nullopt;
        }

        if (entry.depth >= depth) {
            Score score = entry.score;

            /* special case when mating score is found */
            if (score < -s_mateScore)
                score += ply;
            if (score > s_mateScore)
                score -= ply;

            return ProbeResult { .score = score, .flag = entry.flag, .move = entry.move };
        }

        return std::nullopt;
    }

    constexpr static void writeEntry(uint64_t key, Score score, const movegen::Move& move, uint8_t depth, uint8_t ply, TtHashFlag flag)
    {
        assert(s_ttHashSize > 0);

        /* special case when mating score is found */
        if (score < -s_mateScore)
            score -= ply;
        if (score > s_mateScore)
            score += ply;

        auto& entry = s_ttHashTable[key % s_ttHashSize];
        entry = TtHashEntry {
            .key = key,
            .flag = flag,
            .score = score,
            .move = move,
            .depth = depth,
        };
    }

    constexpr static std::size_t tableSizeFromMb(size_t sizeMb)
    {
        return (sizeMb * 1024 * 1024) / sizeof(TtHashEntry);
    }

private:
    static inline std::size_t s_ttHashSize { 0 };
    static inline TtHashEntry* s_ttHashTable;
    static inline std::atomic<uint64_t> s_hashCount {};
};
}
