#pragma once

#include "src/attack_generation.h"
#include "src/bit_board.h"
#include "src/evaluation/history_moves.h"
#include "src/evaluation/killer_moves.h"
#include "src/evaluation/pv_table.h"
#include "src/movement/move_types.h"

#include <cstdint>

namespace evaluation {

class MoveScoring {
public:
    constexpr void reset()
    {
        m_killerMoves.reset();
        m_historyMoves.reset();
        m_pvTable.reset();
    }

    constexpr heuristic::PVTable& pvTable()
    {
        return m_pvTable;
    }

    constexpr heuristic::KillerMoves& killerMoves()
    {
        return m_killerMoves;
    }

    constexpr heuristic::HistoryMoves& historyMoves()
    {
        return m_historyMoves;
    }

    // NOTE: MUST BE CONST - STL SORTING ALGORTIHMS WILL MESS WITH THE STACK IF NOT
    constexpr int32_t score(const BitBoard& board, const movement::Move& move, uint8_t ply) const
    {
        if (m_pvTable.isScoring() && m_pvTable.isPvMove(move, ply)) {
            // return highest value if move is PV
            return 20000;
        }

        switch (move.promotionType()) {
        case PromotionNone:
            break;
        case PromotionQueen:
            return 19000;
        case PromotionKnight:
            return 18000;
        case PromotionBishop:
            return 16000;
        case PromotionRook:
            return 17000;
        }

        const auto attacker = board.getPieceAtSquare(move.fromSquare());
        const auto victim = board.getPieceAtSquare(move.toSquare());

        if (move.isCapture()) {
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
                return m_historyMoves.get(attacker.value(), move.toValue());
            }
        }

        return 0;
    }

private:
    heuristic::KillerMoves m_killerMoves {};
    heuristic::HistoryMoves m_historyMoves {};
    heuristic::PVTable m_pvTable {};
};

}

