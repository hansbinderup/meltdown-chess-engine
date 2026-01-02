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
        const uint64_t threatKey = core::splitMixHash(board.attacks[opponent] & board.pieces[player]);

        const Score kpCorrection = getEntry<player>(m_kpTable, board.kpHash);
        const Score minorCorrection = getEntry<player>(m_minorTable, board.minorHash);
        const Score majorCorrection = getEntry<player>(m_majorTable, board.majorHash);
        const Score pawnCorrection = getEntry<player>(m_pawnTable, board.pawnHash);
        const Score nonPawnCorrectionWhite = getEntry<PlayerWhite>(m_nonPawnTables, board.nonPawnHashes[PlayerWhite]);
        const Score nonPawnCorrectionBlack = getEntry<PlayerBlack>(m_nonPawnTables, board.nonPawnHashes[PlayerBlack]);
        const Score threatCorrection = getEntry<player>(m_threatTable, threatKey);

        const Score correction
            = kpCorrection * spsa::kpCorrectionWeight
            + minorCorrection * spsa::minorCorrectionWeight
            + majorCorrection * spsa::majorCorrectionWeight
            + pawnCorrection * spsa::pawnCorrectionWeight
            + (nonPawnCorrectionWhite + nonPawnCorrectionBlack) * spsa::nonPawnCorrectionWeight
            + threatCorrection * spsa::threatCorrectionWeight;

        return correction / s_grain;
    }

    template<Player player>
    inline void update(const BitBoard& board, uint8_t depth, Score score, Score eval)
    {
        constexpr Player opponent = nextPlayer(player);
        const uint64_t threatKey = core::splitMixHash(board.attacks[opponent] & board.occupation[player]);

        updateEntry<player>(m_kpTable, board.kpHash, score, eval, depth);
        updateEntry<player>(m_minorTable, board.minorHash, score, eval, depth);
        updateEntry<player>(m_majorTable, board.majorHash, score, eval, depth);
        updateEntry<player>(m_pawnTable, board.pawnHash, score, eval, depth);
        updateEntry<PlayerWhite>(m_nonPawnTables, board.nonPawnHashes[PlayerWhite], score, eval, depth);
        updateEntry<PlayerBlack>(m_nonPawnTables, board.nonPawnHashes[PlayerBlack], score, eval, depth);
        updateEntry<player>(m_threatTable, threatKey, score, eval, depth);
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
    /* cache settings */
    constexpr static inline size_t s_cacheKeySize { 16 };
    constexpr static inline uint16_t s_cacheMask { 0xffff };
    constexpr static inline size_t s_cacheSize { 1 << s_cacheKeySize };

    using CorrectionHistoryEntry = std::array<Score, s_cacheSize>;
    using CorrectionTable = std::array<CorrectionHistoryEntry, magic_enum::enum_count<Player>()>;

    template<Player player>
    Score getEntry(const CorrectionTable& table, uint64_t hash) const
    {
        return table[player][hash & s_cacheMask] / s_grain;
    }

    /* updates the correction entry using a weighted mix of current and new data */
    template<Player player>
    void updateEntry(CorrectionTable& table, uint64_t hash, Score bestScore, Score eval, uint8_t depth)
    {
        Score& entry = table[player][hash & s_cacheMask];

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

    CorrectionTable m_kpTable {};
    CorrectionTable m_threatTable {};
    CorrectionTable m_minorTable {};
    CorrectionTable m_majorTable {};
    CorrectionTable m_pawnTable {};
    CorrectionTable m_nonPawnTables {};
};

}
