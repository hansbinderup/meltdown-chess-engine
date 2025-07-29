#pragma once

#include "core/bit_board.h"
#include "core/board_defs.h"
#include "movegen/move_types.h"
#include "spsa/parameters.h"

#include <array>

namespace search {

class ContinuationHistory {
public:
    void reset()
    {
        m_continuationTable = {};
    }

    void update(Piece prevPiece, BoardPosition prevPos, movegen::Move currentMove, Score bestScore, Score eval, uint8_t depth)
    {
        Score& entry = m_continuationTable[prevPiece][prevPos][currentMove.fromPos()][currentMove.toPos()];

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

    Score getScore(Piece prevPiece, BoardPosition prevPos, movegen::Move currentMove) const
    {
        return m_continuationTable[prevPiece][prevPos][currentMove.fromPos()][currentMove.toPos()] / spsa::continuationDivisor;
    }

private:
    using ContinuationHistoryEntry = std::array<std::array<std::array<Score, s_amountSquares>, s_amountSquares>, s_amountSquares>;
    std::array<ContinuationHistoryEntry, magic_enum::enum_count<Piece>()> m_continuationTable {};

    constexpr static inline uint16_t s_grain { 256 };
    constexpr static inline uint16_t s_maxWeight { 256 };
    constexpr static inline uint16_t s_maxValue { 32 * s_grain };
    constexpr static inline uint16_t s_maxUpdate { s_maxValue / 4 };
};

}
