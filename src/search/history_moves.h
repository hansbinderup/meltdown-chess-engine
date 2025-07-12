
#pragma once

#include "core/bit_board.h"
#include "movegen/move_types.h"
#include <algorithm>

namespace search {

class HistoryMoves {
public:
    int32_t get(Piece movePiece, uint8_t targetPosition) const
    {
        return m_historyMoves.at(targetPosition).at(movePiece);
    }

    void update(const BitBoard& board, movegen::Move move, uint8_t ply)
    {
        /* only quiet moves */
        if (move.isCapture()) {
            return; // nothing to do
        }

        const auto attacker = board.getAttackerAtSquare(move.fromSquare(), board.player);

        if (!attacker.has_value())
            assert(false);

        m_historyMoves.at(move.toPos()).at(attacker.value()) += ply;
    }

    inline void addNodes(movegen::Move move, uint64_t nodes)
    {
        m_historyNodes[move.fromPos()][move.toPos()] += nodes;
    }

    inline uint64_t getNodes(movegen::Move move) const
    {
        return m_historyNodes[move.fromPos()][move.toPos()];
    }

    void reset()
    {
        std::ranges::fill(m_historyMoves, HistoryMove {});
    }

    void resetNodes()
    {
        std::ranges::fill(m_historyNodes, HistoryNodes {});
    }

private:
    using HistoryMove = std::array<int32_t, magic_enum::enum_count<Piece>()>;
    std::array<HistoryMove, 64> m_historyMoves {}; // history move for each square

    using HistoryNodes = std::array<uint64_t, s_amountSquares>;
    std::array<HistoryNodes, s_amountSquares> m_historyNodes {};
};

}
