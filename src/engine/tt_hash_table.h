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

/*
 * Non-key members are encoded for compression. From lower bits to higher, entries are:
 * depth: 8 bits, i.e. 0000 ... 0000 0000 1111 1111
 * flag: 2 bits, i.e. 0000 ... 0000 0011 1000 0000
 * score: 16 bits for value + 1 for sign (see board_defs.h)
 * generation: 4 bits
 * move: 26 bits (see move_types.h)
 * */

class TtHashEntryData {
public:
    explicit TtHashEntryData() = default;

    explicit TtHashEntryData(uint64_t depth, uint64_t flag, int64_t score, uint64_t generation, uint64_t move)
        : m_data(
              depth
              | flag << s_flagShift
              // Most significant bit: sign (1 if negative)
              | encodeScore(score)
              | generation << s_generationShift
              | move << s_moveShift)
    {
    }

    constexpr uint8_t getDepth() const
    {
        return m_data & s_depthMask;
    }

    constexpr auto getFlag() const
    {
        return static_cast<TtHashFlag>((m_data >> s_flagShift) & s_flagMask);
    }

    constexpr int32_t getScore() const
    {
        // Casting is safe: operand's MSB is always zero
        auto scoreBits = static_cast<int32_t>((m_data >> s_scoreShift) & s_scoreMask);

        if (scoreBits >= (1 << 16)) {
            // From negative value
            return -(scoreBits - (1 << 16));
        } else {
            return scoreBits;
        }
    }

    constexpr uint8_t getGeneration() const
    {
        return (m_data >> s_generationShift) & s_generationMask;
    }

    constexpr uint32_t getMoveData() const
    {
        return (m_data >> s_moveShift) & s_moveMask;
    }

private:
    constexpr static uint64_t encodeScore(int32_t score)
    {
        constexpr uint64_t scoreSign = 1ULL << s_scoreSignShift;
        return (score >= 0 ? score : -score + scoreSign) << s_scoreShift;
    }

    constexpr static uint64_t s_depthMask { 0xFF };

    constexpr static uint64_t s_flagMask { 0x3 };
    constexpr static uint64_t s_flagShift { 8 };

    constexpr static uint64_t s_scoreSignShift { 16 };
    constexpr static uint64_t s_scoreMask { 0x1FFFF };
    constexpr static uint64_t s_scoreShift { 10 };

    constexpr static uint64_t s_generationMask { 0xF };
    constexpr static uint64_t s_generationShift { 27 };

    constexpr static uint64_t s_moveMask { 0x3FFFFFF };
    constexpr static uint64_t s_moveShift { 31 };

    uint64_t m_data {};
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
        return s_ttHashSize;
    }

    static void clear()
    {
        for (size_t i = 0; i < s_ttHashSize; i++) {
            s_ttHashTable[i].key = 0;
            s_ttHashTable[i].data = TtHashEntryData();
        }

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
        constexpr uint16_t maxValue { 1000 };
        const uint16_t permill = (1000 * s_hashCount.load(std::memory_order_relaxed)) / s_ttHashSize;

        return std::min(permill, maxValue);
    }

    using ProbeResult = std::pair<int32_t, movegen::Move>;
    constexpr static std::optional<ProbeResult> probe(uint64_t key, int32_t alpha, int32_t beta, uint8_t depth, uint8_t ply)
    {
        assert(s_ttHashSize > 0);

        auto& entry = s_ttHashTable[key % s_ttHashSize];

        uint64_t entryKey = entry.key.load(std::memory_order_relaxed);
        auto entryData = entry.data.load(std::memory_order_relaxed);

        if (entryKey != key) {
            return std::nullopt;
        }

        /* Entry is outdated (from a previous generation) */
        if (entryData.getGeneration() != s_currentGeneration) {
            // Can be racy
            if (s_hashCount.load(std::memory_order_relaxed) > 0) {
                --s_hashCount;
            }
            return std::nullopt;
        }

        if (entryData.getDepth() >= depth) {
            int32_t score = entryData.getScore();

            /* special case when mating score is found */
            if (score < -s_mateScore)
                score += ply;
            if (score > s_mateScore)
                score -= ply;

            if (entryData.getFlag() == TtHashExact) {
                return std::make_pair(score, movegen::Move(entryData.getMoveData()));
            }

            if ((entryData.getFlag() == TtHashAlpha) && (score <= alpha)) {
                return std::make_pair(alpha, movegen::Move(entryData.getMoveData()));
            }

            if ((entryData.getFlag() == TtHashBeta) && (score >= beta)) {
                return std::make_pair(beta, movegen::Move(entryData.getMoveData()));
            }
        }

        return std::nullopt;
    }

    constexpr static void writeEntry(uint64_t key, int32_t score, const movegen::Move& move, uint8_t depth, uint8_t ply, TtHashFlag flag)
    {
        assert(s_ttHashSize > 0);

        /* special case when mating score is found */
        if (score < -s_mateScore)
            score -= ply;
        if (score > s_mateScore)
            score += ply;

        auto& entry = s_ttHashTable[key % s_ttHashSize];

        if (entry.key.load(std::memory_order_relaxed) == 0) {
            ++s_hashCount;
        }

        TtHashEntryData newData { depth, flag, score, s_currentGeneration, move.getData() };

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
    constexpr static uint8_t s_generationMask { 0xF }; /* 4-bit wrapping generation (max 16 generations allowed) */

    static inline TtHashEntry* s_ttHashTable;
    static inline std::atomic<uint64_t> s_hashCount {};
    static inline uint8_t s_currentGeneration {};
};
}
