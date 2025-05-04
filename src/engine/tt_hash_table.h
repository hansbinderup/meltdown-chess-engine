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
    uint16_t key16 = 0;
    TtHashFlag flag;
    Score score {};
    movegen::Move move {};
    uint8_t depth {};
};

namespace {

constexpr inline uint16_t key16From64(uint64_t key)
{
    return key >> 48;
}

}

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
        uint16_t count = 0;

        /* hash table access is "random" so it should be
         * a good enough estimate to just check 1000 random entries
         *
         * 1000 because we're calculating permille */
        for (uint16_t i = 0; i < 1000; i++) {
            if (s_ttHashTable[i].key16 != 0)
                count++;
        }

        return count;
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

        if (entry.key16 != key16From64(key)) {
            return std::nullopt;
        }

        if (entry.depth >= depth) {
            return ProbeResult {
                .score = scoreRelative(entry.score, ply),
                .flag = entry.flag,
                .move = entry.move,
            };
        }

        return std::nullopt;
    }

    constexpr static void writeEntry(uint64_t key, Score score, const movegen::Move& move, uint8_t depth, uint8_t ply, TtHashFlag flag)
    {
        assert(s_ttHashSize > 0);

        const uint16_t key16 = key16From64(key);

        auto& entry = s_ttHashTable[key % s_ttHashSize];
        entry = TtHashEntry {
            .key16 = key16,
            .flag = flag,
            .score = scoreAbsolute(score, ply),
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
