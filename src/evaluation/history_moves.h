
#pragma once

#include "bit_board.h"
#include "movegen/move_types.h"
#include <algorithm>

namespace evaluation {

class HistoryMoves {
public:
    int32_t get(Piece movePiece, uint8_t targetPosition) const
    {
        return m_historyMoves.at(targetPosition).at(movePiece);
    }

    void update(const BitBoard& board, const movegen::Move& move, uint8_t ply)
    {
        /* only quiet moves */
        if (move.isCapture()) {
            return; // nothing to do
        }

        const auto movePiece = board.getPieceAtSquare(move.fromSquare());

        if (!movePiece.has_value())
            return; // nothing to do

        m_historyMoves.at(move.toValue()).at(movePiece.value()) += ply;
    }

    void reset()
    {
        std::ranges::fill(m_historyMoves, HistoryMove {});
    }

private:
    using HistoryMove = std::array<int32_t, magic_enum::enum_count<Piece>()>;
    std::array<HistoryMove, 64> m_historyMoves {}; // history move for each square
};

}

