#pragma once

#include "evaluation/term_score.h"

#include <array>

namespace evaluation {

struct KingPawnEntry {
    uint64_t key {};
    TermScore score = TermScore(0, 0);
    std::array<uint64_t, 2> passedPawns {};
};

/* simple storage to save and read KingPawnEntries
 * NOTE: should not be used multi threaded
 * NOTE: storage is relatively small but it still results
 *       in about 95% cache hits */
class KingPawnCache {
public:
    inline void write(const KingPawnEntry& newEntry)
    {
        auto& entry = m_table[newEntry.key & s_cacheMask];
        entry = newEntry;
    }

    inline std::optional<KingPawnEntry> probe(uint64_t key) const
    {
        const auto& entry = m_table[key & s_cacheMask];
        return key == entry.key ? std::make_optional(entry) : std::nullopt;
    }

private:
    constexpr static inline size_t s_cacheKeySize { 16 };
    constexpr static inline uint16_t s_cacheMask { 0xffff };
    constexpr static inline size_t s_cacheSize { 1 << s_cacheKeySize };

    std::array<KingPawnEntry, s_cacheSize> m_table {};
};

}
