#pragma once

#include "core/bit_board.h"
#include "core/zobrist_hashing.h"
#include "evaluation/score.h"
#include "spsa/parameters.h"

#include <array>

namespace search {

class CorrectionHistory {
public:
    /* computes the combined correction score for a board by mixing
     * corrections with their respective weights */
    inline Score getCorrection(const BitBoard& board) const
    {
        const Score kpCorrection = getCorrection(board.player, board.kpHash);
        const Score materialCorrection = getCorrection(board.player, core::generateMaterialHash(board));

        const Score correction
            = kpCorrection * spsa::pawnCorrectionWeight
            + materialCorrection * spsa::materialCorrectionWeight;

        return correction / s_grain;
    }

    inline void update(const BitBoard& board, uint8_t depth, Score score, Score eval)
    {
        updateEntry(board.player, board.kpHash, score, eval, depth);
        updateEntry(board.player, core::generateMaterialHash(board), score, eval, depth);
    }

private:
    /* returns the scaled correction value for a given player and hash key */
    Score getCorrection(Player player, uint64_t hash) const
    {
        return m_table[player][hash & s_cacheMask] / s_grain;
    }

    /* updates the correction entry using a weighted mix of current and new data */
    void updateEntry(Player player, uint64_t hash, Score bestScore, Score eval, uint8_t depth)
    {
        Score& entry = m_table[player][hash & s_cacheMask];

        const Score diff = (bestScore - eval) * s_grain;
        const Score newWeight = std::min<Score>(depth + 1, 16);
        const Score oldWeight = s_maxWeight - newWeight;
        Score updated = (entry * oldWeight * diff * newWeight) / s_maxWeight;

        /* clamp update to avoid excessive changes */
        const Score minUpdate = entry - s_maxUpdate;
        const Score maxUpdate = entry + s_maxUpdate;

        updated = std::clamp(updated, minUpdate, maxUpdate); /* first clamp delta */
        updated = std::clamp<Score>(updated, -s_maxValue, s_maxValue); /* then clamp absolute */

        entry = updated;
    }

    /* scaling factors for corrections */
    constexpr static inline uint16_t s_grain { 256 };
    constexpr static inline uint16_t s_maxWeight { 256 };
    constexpr static inline uint16_t s_maxValue { 32 * s_grain };
    constexpr static inline uint16_t s_maxUpdate { s_maxValue / 4 };

    /* cache settings */
    constexpr static inline size_t s_cacheKeySize { 16 };
    constexpr static inline uint16_t s_cacheMask { 0xffff };
    constexpr static inline size_t s_cacheSize { 1 << s_cacheKeySize };

    using CorrectionHistoryEntry = std::array<Score, s_cacheSize>;
    std::array<CorrectionHistoryEntry, magic_enum::enum_count<Player>()> m_table {};
};

}

