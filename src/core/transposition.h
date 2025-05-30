#pragma once

#include "fmt/base.h"
#include "movegen/move_types.h"
#include "utils/memory.h"
#include <atomic>
#include <cassert>
#include <cstdint>

namespace core {

enum TtFlag : uint8_t {
    TtExact = 0,
    TtAlpha = 1,
    TtBeta = 2,
};

struct TtEntryData {
    uint8_t depth;
    TtFlag flag;
    Score score;
    Score eval;
    movegen::Move move;
};

static_assert(std::atomic<TtEntryData>::is_always_lock_free);

struct TtEntry {
    std::atomic<uint64_t> key { 0 };
    std::atomic<TtEntryData> data;
};

constexpr inline std::optional<Score> testEntry(const TtEntryData& entryData, uint8_t ply, uint8_t depth, Score alpha, Score beta)
{
    if (entryData.depth < depth || entryData.score == s_noScore)
        return std::nullopt;

    const Score relScore = scoreRelative(entryData.score, ply);

    if (entryData.flag == TtExact)
        return relScore;
    else if (entryData.flag == TtAlpha && relScore <= alpha)
        return relScore;
    else if (entryData.flag == TtBeta && relScore >= beta)
        return relScore;

    return std::nullopt;
}

class TranspositionTable {
public:
    /* frees the memory of the current table (if any) and allocates the amount provided in MB
     * NOTE: NOT THREAD SAFE */
    static void setSizeMb(std::size_t sizeMb)
    {
        if (sizeMb <= 0) {
            fmt::println("Invalid size: {}mb", sizeMb);
            return;
        }

        if (s_tableSize > 0) {
            utils::alignedFree(s_table);
        }

        s_tableSize = tableSizeFromMb(sizeMb);
        s_table = static_cast<TtEntry*>(utils::alignedAlloc(alignof(TtEntry), s_tableSize * sizeof(TtEntry)));

        if (s_table == nullptr) {
            fmt::println("Could not allocate memory for hash table");
            s_tableSize = 0;
        }

        clear();
    }

    /* prefetches the memory and loads it into L1 cache line */
    constexpr static void prefetch([[maybe_unused]] uint64_t key)
    {
        assert(s_tableSize > 0);
        __builtin_prefetch(&s_table[key % s_tableSize]);
    }

    static std::size_t getSizeMb()
    {
        return (s_tableSize / 1024 / 1024) * sizeof(TtEntry);
    }

    static void clear()
    {
        for (size_t i = 0; i < s_tableSize; i++) {
            s_table[i].key = 0;
            s_table[i].data = TtEntryData();
        }
    }

    static uint16_t getHashFull()
    {
        assert(s_tableSize >= 1000);

        /* should be returned as permill */
        uint16_t permill = 0;

        /* table access is pseudo random so the most efficient way of finding
         * the permill occupation must be accessing 1000 random entries and
         * check if they're already written to */
        for (uint16_t i = 0; i < 1000; i++) {
            if (s_table[i].key.load(std::memory_order_relaxed) != 0) {
                permill++;
            }
        }

        return permill;
    }

    constexpr static std::optional<TtEntryData> probe(uint64_t key)
    {
        assert(s_tableSize > 0);

        auto& entry = s_table[key % s_tableSize];

        uint64_t entryKey = entry.key.load(std::memory_order_relaxed);
        auto entryData = entry.data.load(std::memory_order_relaxed);

        if (entryKey != key) {
            return std::nullopt;
        }

        return entryData;
    }

    constexpr static void writeEntry(uint64_t key, Score score, Score eval, movegen::Move move, uint8_t depth, uint8_t ply, TtFlag flag)
    {
        assert(s_tableSize > 0);

        auto& entry = s_table[key % s_tableSize];
        const auto entryKey = entry.key.load(std::memory_order_relaxed);
        const auto entryData = entry.data.load(std::memory_order_relaxed);

        /* only update entry if a better one is found */
        if (key != entryKey
            || entryData.move.isNull()
            || depth >= entryData.depth
            || (flag == TtExact && entryData.flag != TtExact)) {

            TtEntryData newData {
                .depth = depth,
                .flag = flag,
                .score = scoreAbsolute(score, ply),
                .eval = eval,
                .move = move,
            };

            // Can be racy
            entry.key.store(key, std::memory_order_relaxed);
            entry.data.store(newData, std::memory_order_relaxed);
        }
    }

    constexpr static std::size_t tableSizeFromMb(size_t sizeMb)
    {
        return (sizeMb * 1024 * 1024) / sizeof(TtEntry);
    }

private:
    static inline std::size_t s_tableSize { 0 };
    static inline TtEntry* s_table;
};
}
