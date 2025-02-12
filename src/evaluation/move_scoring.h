#pragma once

#include "src/attack_generation.h"
#include "src/bit_board.h"
#include "src/evaluation/history_moves.h"
#include "src/evaluation/killer_moves.h"
#include "src/movement/move_types.h"

#include <cstdint>

namespace evaluation {

class MoveScoring {
public:
    constexpr static inline void resetHeuristics()
    {
        m_killerMoves.reset();
        m_historyMoves.reset();
    }

    constexpr static inline void updateKillerMove(const movement::Move& move, uint8_t ply)
    {
        m_killerMoves.update(move, ply);
    }

    constexpr static inline void updateHistoryMove(const BitBoard& board, const movement::Move& move, uint8_t ply)
    {
        m_historyMoves.update(board, move, ply);
    }

    constexpr static inline int16_t score(const BitBoard& board, const movement::Move& move, uint8_t ply)
    {
        const auto attacker = board.getPieceAtSquare(move.fromSquare());
        const auto victim = board.getPieceAtSquare(move.toSquare());

        if (magic_enum::enum_flags_test(move.flags, movement::MoveFlags::Capture)) {
            if (attacker.has_value() && victim.has_value()) {
                return gen::getMvvLvaScore(attacker.value(), victim.value()) + 10000;
            }
        } else {
            const auto killerMoves = m_killerMoves.get(ply);
            if (move == killerMoves.first)
                return 9000;
            else if (move == killerMoves.second)
                return 8000;
            else if (attacker.has_value()) {
                return m_historyMoves.get(attacker.value(), move.to, board.player);
            }
        }

        return 0;
    }

private:
    static inline heuristic::KillerMoves m_killerMoves;
    static inline heuristic::HistoryMoves m_historyMoves;
};

}

