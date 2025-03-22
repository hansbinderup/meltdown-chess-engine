#pragma once

#include "board_defs.h"
#include "movegen/move_types.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>

namespace engine {

enum TtHashFlag : uint8_t {
    TtHashExact = 0,
    TtHashAlpha = 1,
    TtHashBeta = 2,
};

/*
 * Non-key members are encoded for compression. From lower bits to higher, entries are:
 * depth: 7 bits, i.e. 0000 ... 0000 0000 0111 1111
 * flag: 3 bits, i.e. 0000 ... 0000 0011 1000 0000
 * score: 17 bits (see board_defs.h)
 * generation: 8 bits
 * move: 26 bits (see move_types.h)
 * */

class TtHashEntryData {
public:
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshift-count-overflow"
    explicit TtHashEntryData(uint64_t depth = 0, uint64_t flag = TtHashExact, int64_t score = 0, uint64_t generation = 0, uint64_t move = {})
        : m_data(
              depth
              | flag << s_flagShift
              | score << s_scoreShift
              | generation << s_generationShift
              | move << s_moveShift)
    {
    }
#pragma GCC diagnostic pop

    constexpr inline uint8_t getDepth() const
    {
        return m_data & s_depthMask;
    }

    constexpr inline auto getFlag() const
    {
        return static_cast<TtHashFlag>((m_data >> s_flagShift) & s_flagMask);
    }

    constexpr inline int32_t getScore() const
    {
        return (m_data >> s_scoreShift) & s_scoreMask;
    }

    constexpr inline uint8_t getGeneration() const
    {
        return (m_data >> s_generationShift) & s_generationMask;
    }

    constexpr inline uint32_t getMoveData() const
    {
        return (m_data >> s_moveShift) & s_moveMask;
    }

private:
    constexpr static inline uint64_t s_depthMask { 0b1111111 };

    constexpr static inline uint64_t s_flagMask { 0b111 };
    constexpr static inline uint64_t s_flagShift { 7 };

    constexpr static inline uint64_t s_scoreMask { 0b11111111111111111 };
    constexpr static inline uint64_t s_scoreShift { 10 };

    constexpr static inline uint64_t s_generationMask { 0b11111111 };
    constexpr static inline uint64_t s_generationShift { 27 };

    constexpr static inline uint64_t s_moveMask { 0b11111111111111111111111111 };
    constexpr static inline uint64_t s_moveShift { 35 };

    uint64_t m_data;
};

struct TtHashEntry {
    std::atomic<uint64_t> key { 0 };
    std::atomic<TtHashEntryData> data;
};

class TtHashTable {
public:
    static void clear()
    {

        for (auto& entry : s_ttHashTable) {
            entry.key = 0;
            entry.data = TtHashEntryData(); // Manually construct
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
        return std::min((1000 * s_hashCount) / s_ttHashSize, 1000UL);
    }

    using ProbeResult = std::pair<int32_t, movegen::Move>;
    constexpr static std::optional<ProbeResult> probe(uint64_t key, int32_t alpha, int32_t beta, uint8_t depth, uint8_t ply)
    {
        auto& entry = s_ttHashTable[key % s_ttHashSize];

        uint64_t entryKey = entry.key.load(std::memory_order_relaxed);
        auto entryData = entry.data.load(std::memory_order_relaxed);

        if (entryKey != key) {
            return std::nullopt;
        }

        /* Entry is outdated (from a previous generation) */
        if (entryData.getGeneration() != s_currentGeneration) {
            // Can be racy
            if (s_hashCount > 0) {
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
        static_assert(std::atomic<uint64_t>::is_always_lock_free);
        entry.key.store(key, std::memory_order_relaxed);
        entry.data.store(newData, std::memory_order_relaxed);
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
