#pragma once

#include "core/bit_board.h"
#include "core/zobrist_hashing.h"
#include "evaluation/score.h"
#include "spsa/parameters.h"

#include <array>

namespace search {

/* CorrectionHistory maintains correction tables that store evaluation
 * adjustments for recurring board features such as material and pawn structure
 * These corrections help refine the static evaluation by incorporating feedback
 * from past search results, enabling the engine to learn from its previous mistakes
 * and adapt its evaluations accordingly
 *
 * inspired by https://www.chessprogramming.org/Static_Evaluation_Correction_History */

class CorrectionHistory {
public:
    inline Score getCorrection(const BitBoard& board) const
    {
        if (board.player == PlayerWhite) {
            return getCorrection<PlayerWhite>(board);
        } else {
            return getCorrection<PlayerBlack>(board);
        }
    }

    /* computes the combined correction score for a board by mixing
     * corrections with their respective weights */
    template<Player player>
    inline Score getCorrection(const BitBoard& board) const
    {
        constexpr Player opponent = nextPlayer(player);
        const uint64_t threatKey = core::splitMixHash(board.attacks[opponent] & board.occupation[player]);

        const Score kpCorrection = getEntry<player>(board.kpHash);
        const Score materialCorrection = getEntry<player>(core::generateMaterialHash(board));
        const Score threatCorrection = getEntry<player>(threatKey);
        const Score wNonPawnCorrection = getEntry<player>(core::generateNonPawnHash<PlayerWhite>(board));
        const Score bNonPawnCorrection = getEntry<player>(core::generateNonPawnHash<PlayerBlack>(board));

        const Score correction
            = kpCorrection * spsa::pawnCorrectionWeight
            + materialCorrection * spsa::materialCorrectionWeight
            + threatCorrection * spsa::threatCorrectionWeight
            + wNonPawnCorrection * spsa::nonPawnCorrectionWeight
            + bNonPawnCorrection * spsa::nonPawnCorrectionWeight;

        return correction / s_grain;
    }

    template<Player player>
    inline void update(const BitBoard& board, uint8_t depth, Score score, Score eval)
    {
        constexpr Player opponent = nextPlayer(player);
        const uint64_t threatKey = core::splitMixHash(board.attacks[opponent] & board.occupation[player]);

        updateEntry<player>(board.kpHash, score, eval, depth);
        updateEntry<player>(core::generateMaterialHash(board), score, eval, depth);
        updateEntry<player>(threatKey, score, eval, depth);
        updateEntry<player>(core::generateNonPawnHash<PlayerWhite>(board), score, eval, depth);
        updateEntry<player>(core::generateNonPawnHash<PlayerBlack>(board), score, eval, depth);
    }

    inline void update(const BitBoard& board, uint8_t depth, Score score, Score eval)
    {
        if (board.player == PlayerWhite) {
            return update<PlayerWhite>(board, depth, score, eval);
        } else {
            return update<PlayerBlack>(board, depth, score, eval);
        }
    }

private:
    template<Player player>
    Score getEntry(uint64_t hash) const
    {
        return m_table[player][hash & s_cacheMask] / s_grain;
    }

    /* updates the correction entry using a weighted mix of current and new data */
    template<Player player>
    void updateEntry(uint64_t hash, Score bestScore, Score eval, uint8_t depth)
    {
        Score& entry = m_table[player][hash & s_cacheMask];

        /* mix previous correction with new data:
         * - compute difference between best score and evaluation
         * - weight it based on depth (higher depth = more trust)
         * - combine with existing entry using weighted average
         *
         * newWeight limited to 16 to avoid overfitting when
         * performing deep searches. (16/256)=>6%~ depth influence
         * which should be fine for both shallow and deeper searches
         *
         * NOTE: we also add +1 to avoid cancelling out at depth=0 */

        const Score diff = (bestScore - eval) * s_grain;
        const Score newWeight = std::min<Score>(depth + 1, 16);
        const Score oldWeight = s_maxWeight - newWeight;
        Score updated = (entry * oldWeight + diff * newWeight) / s_maxWeight;

        /* update limits to avoid excessive changes */
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
