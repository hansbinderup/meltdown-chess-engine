#pragma once

#include "core/bit_board.h"
#include "evaluation/score.h"
#include "spsa/parameters.h"
#include <array>

namespace search {

class CorrectionHistory {
public:
    inline Score getCorrection(const BitBoard& board) const
    {
        const Score kpCorrection = getCorrection(board.player, board.kpHash);

        /* FIXME: add more correction here */
        const Score corretion = kpCorrection * spsa::pawnCorrectionWeight;

        return corretion / s_grain;
    }

    inline void update(const BitBoard& board, uint8_t depth, Score score, Score eval)
    {
        /* FIXME: add more hashes here */
        updateEntry(m_table[board.player][board.kpHash & s_cacheMask], score, eval, depth);
    }

private:
    Score getCorrection(Player player, uint64_t hash) const
    {
        return m_table[player][hash & s_cacheMask] / s_grain;
    }

    void updateEntry(Score& entry, Score bestScore, Score eval, uint8_t depth)
    {
        const Score diff = (bestScore - eval) * s_grain;
        const Score newWeight = std::min<Score>(depth + 1, 16);
        const Score oldWeight = s_maxWeight - newWeight;
        Score updated = (entry * oldWeight * diff * newWeight) / s_maxWeight;

        const Score minUpdate = entry - s_maxUpdate;
        const Score maxUpdate = entry + s_maxUpdate;

        updated = std::clamp(updated, minUpdate, maxUpdate);
        updated = std::clamp<Score>(updated, -s_maxValue, s_maxValue);

        entry = updated;
    }

    constexpr static inline uint16_t s_grain { 256 };
    constexpr static inline uint16_t s_maxWeight { 256 };
    constexpr static inline uint16_t s_maxValue { 32 * s_grain };
    constexpr static inline uint16_t s_maxUpdate { s_maxValue / 4 };

    constexpr static inline size_t s_cacheKeySize { 16 };
    constexpr static inline uint16_t s_cacheMask { 0xffff };
    constexpr static inline size_t s_cacheSize { 1 << s_cacheKeySize };

    using CorrectionHistoryEntry = std::array<Score, s_cacheSize>;
    std::array<CorrectionHistoryEntry, magic_enum::enum_count<Player>()> m_table {};
};

}

