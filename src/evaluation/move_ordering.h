#pragma once

#include "bit_board.h"
#include "evaluation/history_moves.h"
#include "evaluation/killer_moves.h"
#include "evaluation/pv_table.h"
#include "evaluation/see_swap.h"
#include "movegen/move_types.h"

#include <cstdint>

namespace evaluation {

namespace {

enum ScoringOffsets : int32_t {
    ScoreTtHashMove = 25000,
    ScorePvLine = 20000,
    ScoreGoodCapture = 10000,
    ScoreBadCapture = -10000,
    ScoreGoodPromotion = 8000,
    ScoreBadPromotion = -8000,
    ScoreKillerMove = 2000,
    ScoreHistoryMove = -2000,
};

}

class MoveOrdering {
public:
    constexpr void reset()
    {
        m_killerMoves.reset();
        m_historyMoves.reset();
        m_pvTable.reset();
    }

    constexpr PVTable& pvTable()
    {
        return m_pvTable;
    }

    constexpr const PVTable& pvTable() const
    {
        return m_pvTable;
    }

    constexpr KillerMoves& killerMoves()
    {
        return m_killerMoves;
    }

    constexpr HistoryMoves& historyMoves()
    {
        return m_historyMoves;
    }

    template<Player player>
    constexpr void sortMoves(const BitBoard& board, movegen::ValidMoves& moves, uint8_t ply, std::optional<movegen::Move> ttMove = std::nullopt)
    {
        /* primitive insertion sort */
        for (size_t i = 1; i < moves.count(); ++i) {
            movegen::Move key = moves[i];

            int keyScore = moveScore<player>(board, key, ply, ttMove);
            size_t j = i;
            while (j > 0 && moveScore<player>(board, moves[j - 1], ply, ttMove) < keyScore) {
                moves[j] = moves[j - 1];
                --j;
            }
            moves[j] = key;
        }
        m_pvTable.setIsScoring(false);
    }

    // Helper: calling inside loops will mean redundant colour checks

    constexpr void sortMoves(const BitBoard& board, movegen::ValidMoves& moves, uint8_t ply, std::optional<movegen::Move> ttMove = std::nullopt)
    {
        if (board.player == PlayerWhite) {
            sortMoves<PlayerWhite>(board, moves, ply, ttMove);
        } else {
            sortMoves<PlayerBlack>(board, moves, ply, ttMove);
        }
    }
    // NOTE: scoring must be consistent during a sort
    template<Player player>
    constexpr int32_t moveScore(const BitBoard& board, const movegen::Move& move, uint8_t ply, std::optional<movegen::Move> ttMove = std::nullopt) const
    {
        if (ttMove.has_value() && move == *ttMove) {
            return ScoreTtHashMove;
        }

        if (m_pvTable.isScoring() && m_pvTable.isPvMove(move, ply)) {
            return ScorePvLine;
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

            const auto killerMoves = m_killerMoves.get(ply);
            if (move == killerMoves.first)
                return ScoreKillerMove;
            else if (move == killerMoves.second)
                return ScoreKillerMove - 1000;
            else {
                const auto attacker = board.getAttackerAtSquare<player>(move.fromSquare());
                if (attacker.has_value()) {
                    return ScoreHistoryMove + m_historyMoves.get(attacker.value(), move.toPos());
                }
            }
        }

        return 0;
    }

    // Helper: calling inside loops will mean redundant colour checks
    constexpr int32_t moveScore(const BitBoard& board, const movegen::Move& move, uint8_t ply, std::optional<movegen::Move> ttMove = std::nullopt) const
    {
        if (board.player == PlayerWhite) {
            return moveScore<PlayerWhite>(board, move, ply, ttMove);
        } else {
            return moveScore<PlayerBlack>(board, move, ply, ttMove);
        }
    }

private:
    KillerMoves m_killerMoves {};
    HistoryMoves m_historyMoves {};
    PVTable m_pvTable {};
};
}
