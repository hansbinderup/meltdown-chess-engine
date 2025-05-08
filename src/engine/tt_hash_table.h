#pragma once

#include "fmt/base.h"
#include "helpers/memory.h"
#include "movegen/move_types.h"
#include <atomic>
#include <cassert>
#include <cstdint>

namespace engine {

enum TtHashFlag : uint8_t {
    TtHashExact = 0,
    TtHashAlpha = 1,
    TtHashBeta = 2,
};

/* fixme: remove alignas when entry is 8 bytes */
struct alignas(8) TtHashEntryData {
    uint8_t depth;
    TtHashFlag flag;
    Score score;
    movegen::Move move;
};

static_assert(std::atomic<TtHashEntryData>::is_always_lock_free);

struct TtHashEntry {
    std::atomic<uint64_t> key { 0 };
    std::atomic<TtHashEntryData> data;
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

    /* prefetches the memory and loads it into L1 cache line */
    constexpr static void prefetch([[maybe_unused]] uint64_t key)
    {
        assert(s_ttHashSize > 0);
        __builtin_prefetch(&s_ttHashTable[key % s_ttHashSize]);
    }

    static std::size_t getSizeMb()
    {
        return (s_ttHashSize / 1024 / 1024) * sizeof(TtHashEntry);
    }

    static void clear()
    {
        for (size_t i = 0; i < s_ttHashSize; i++) {
            s_ttHashTable[i].key = 0;
            s_ttHashTable[i].data = TtHashEntryData();
        }
    }

    static uint16_t getHashFull()
    {
        assert(s_ttHashSize >= 1000);

        /* should be returned as permill */
        uint16_t permill = 0;

        /* table access is pseudo random so the most efficient way of finding
         * the permill occupation must be accessing 1000 random entries and
         * check if they're already written to */
        for (uint16_t i = 0; i < 1000; i++) {
            if (s_ttHashTable[i].key.load(std::memory_order_relaxed) != 0) {
                permill++;
            }
        }

        return permill;
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

        uint64_t entryKey = entry.key.load(std::memory_order_relaxed);
        auto entryData = entry.data.load(std::memory_order_relaxed);

        if (entryKey != key) {
            return std::nullopt;
        }

        if (entryData.depth >= depth) {
            return ProbeResult {
                .score = scoreRelative(entryData.score, ply),
                .flag = entryData.flag,
                .move = entryData.move,
            };
        }

        return std::nullopt;
    }

    constexpr static void writeEntry(uint64_t key, Score score, const movegen::Move& move, uint8_t depth, uint8_t ply, TtHashFlag flag)
    {
        assert(s_ttHashSize > 0);

        auto& entry = s_ttHashTable[key % s_ttHashSize];

        TtHashEntryData newData {
            .depth = depth,
            .flag = flag,
            .score = scoreAbsolute(score, ply),
            .move = move,
        };

        // Can be racy
        entry.key.store(key, std::memory_order_relaxed);
        entry.data.store(newData, std::memory_order_relaxed);
    }

    constexpr static std::size_t tableSizeFromMb(size_t sizeMb)
    {
        return (sizeMb * 1024 * 1024) / sizeof(TtHashEntry);
    }

private:
    static inline std::size_t s_ttHashSize { 0 };
    static inline TtHashEntry* s_ttHashTable;
};
}
