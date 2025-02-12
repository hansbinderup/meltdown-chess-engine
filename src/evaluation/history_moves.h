
#pragma once

#include "src/bit_board.h"
#include "src/movement/move_types.h"
#include <algorithm>
#include <utility>

namespace heuristic {

class HistoryMoves {
public:
    int16_t get(Piece movePiece, uint8_t targetPosition, Player player)
    {
        if (player == Player::White) {
            return m_historyMoves.at(targetPosition).at(movePiece);

        } else {
            return m_historyMoves.at(targetPosition).at(movePiece + magic_enum::enum_count<Piece>());
        }
    }

    void update(const BitBoard& board, const movement::Move& move, uint8_t ply)
    {
        if (!magic_enum::enum_flags_test(move.flags, movement::MoveFlags::Capture)) {
            return; // nothing to do
        }

        const auto movePiece = board.getPieceAtSquare(move.fromSquare());

        if (!movePiece.has_value())
            return; // nothing to do

        if (board.player == Player::White) {
            m_historyMoves.at(move.to).at(movePiece.value()) += ply;
        } else {
            m_historyMoves.at(move.to).at(movePiece.value() + magic_enum::enum_count<Piece>()) += ply;
        }
    }

    void reset()
    {
        std::ranges::fill(m_historyMoves, HistoryMove {});
    }

private:
    using HistoryMove = std::array<int16_t, magic_enum::enum_count<Piece>() * 2>;
    std::array<HistoryMove, 64> m_historyMoves; // history move for each square
};

}

