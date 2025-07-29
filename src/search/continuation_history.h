#pragma once

#include "core/bit_board.h"
#include "core/board_defs.h"
#include "movegen/move_types.h"

#include <array>

namespace search {

class ContinuationHistory {
public:
    void reset()
    {
        m_continuationTable = {};
    }

    void update(Piece prevPiece, BoardPosition prevPos, movegen::Move currentMove, Score score)
    {
        m_continuationTable[prevPiece][prevPos][currentMove.fromPos()][currentMove.toPos()] += score;
    }

    Score getScore(Piece prevPiece, BoardPosition prevPos, movegen::Move currentMove) const
    {
        return m_continuationTable[prevPiece][prevPos][currentMove.fromPos()][currentMove.toPos()];
    }

private:
    using ContinuationHistoryEntry = std::array<std::array<std::array<Score, s_amountSquares>, s_amountSquares>, s_amountSquares>;
    std::array<ContinuationHistoryEntry, magic_enum::enum_count<Piece>()> m_continuationTable {};
};

}
