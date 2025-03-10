#pragma once

#include "bit_board.h"
#include "evaluation/history_moves.h"
#include "evaluation/killer_moves.h"
#include "evaluation/pv_table.h"
#include "evaluation/see_swap.h"
#include "movement/move_types.h"

#include <cstdint>

namespace evaluation {

namespace {

enum ScoringOffsets : int32_t {
    ScoreTtHashMove = 25000,
    ScoreGoodCapture = 10000,
    ScoreBadCapture = -10000,
    ScoreGoodPromotion = 8000,
    ScoreBadPromotion = -8000,
    ScoreKillerMove = 2000,
    ScoreHistoryMove = -2000,
};

}

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

    constexpr const heuristic::PVTable& pvTable() const
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

    // NOTE: SCORING MUST BE CONSISTENT DURING A SORT - STL SORTING ALGORTIHMS WILL MESS WITH THE STACK IF NOT
    constexpr int32_t score(const BitBoard& board, const movement::Move& move, uint8_t ply, std::optional<movement::Move> ttMove = std::nullopt) const
    {
        if (ttMove.has_value() && move == *ttMove) {
            return ScoreTtHashMove;
        }

        if (move.isCapture()) {
            int32_t seeScore = evaluation::SeeSwap::run(board, move);

            if (seeScore >= 0) {
                return ScoreGoodCapture + seeScore;
            } else {
                return ScoreBadCapture + seeScore;
            }
        } else {
            switch (move.promotionType()) {
            case PromotionNone:
                break;
            case PromotionQueen:
                return ScoreGoodPromotion;
            case PromotionKnight:
                return ScoreBadPromotion + 1000;
            case PromotionBishop:
                return ScoreBadPromotion;
            case PromotionRook:
                return ScoreBadPromotion + 2000;
            }

            const auto attacker = board.getPieceAtSquare(move.fromSquare());
            const auto killerMoves = m_killerMoves.get(ply);
            if (move == killerMoves.first)
                return ScoreKillerMove;
            else if (move == killerMoves.second)
                return ScoreKillerMove - 1000;
            else if (attacker.has_value()) {
                return ScoreHistoryMove + m_historyMoves.get(attacker.value(), move.toValue());
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

