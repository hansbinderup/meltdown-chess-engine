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
            return m_scoreTtHashMove;
        }

        if (m_pvTable.isScoring() && m_pvTable.isPvMove(move, ply)) {
            return m_scorePvLine;
        }

        if (move.isCapture()) {
            int32_t seeScore = evaluation::SeeSwap::run(board, move);

            if (seeScore >= 0) {
                return m_scoreGoodCapture + seeScore;
            } else {
                return m_scoreBadCapture + seeScore;
            }
        } else {
            switch (move.promotionType()) {
            case PromotionNone:
                break;
            case PromotionQueen:
                return m_scoreGoodPromotion;
            case PromotionKnight:
                return m_scoreBadPromotion + 1000;
            case PromotionBishop:
                return m_scoreBadPromotion;
            case PromotionRook:
                return m_scoreBadPromotion;
            }

            const auto killerMoves = m_killerMoves.get(ply);
            if (move == killerMoves.first)
                return m_scoreKillerMove;
            else if (move == killerMoves.second)
                return m_scoreKillerMove - 1000;
            else {
                const auto attacker = board.getAttackerAtSquare<player>(move.fromSquare());
                if (attacker.has_value()) {
                    return m_scoreHistoryMove + m_historyMoves.get(attacker.value(), move.toPos());
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

    constexpr void generateOffsets(uint8_t threadId)
    {
        m_randSeed = static_cast<uint32_t>(threadId) * 0x9E3779B1;

        // TODO bloated, don't like manual matching to number of move scores below
        std::array<int, 6> offsets { 0, 5, 10, -3, -2, 5 };

        // TODO better PRNG
        for (int32_t& offset : offsets) {
            offset ^= m_randSeed;
            offset %= s_offsetRange;
        }

        m_scoreTtHashMove += offsets.at(0);
        m_scorePvLine += offsets.at(1);
        m_scoreGoodCapture += offsets.at(2);
        m_scoreBadCapture += offsets.at(2);
        m_scoreGoodPromotion += offsets.at(3);
        m_scoreBadPromotion += offsets.at(3);
        m_scoreKillerMove += offsets.at(4);
        m_scoreHistoryMove
            += offsets.at(5);
    }

private:
    mutable uint32_t m_randSeed { 0 };
    KillerMoves m_killerMoves {};
    HistoryMoves m_historyMoves {};
    PVTable m_pvTable {};

    // TODO better data structure?
    int32_t m_scoreTtHashMove { ScoringOffsets::ScoreTtHashMove };
    int32_t m_scorePvLine { ScoringOffsets::ScorePvLine };
    int32_t m_scoreGoodCapture { ScoringOffsets::ScoreGoodCapture };
    int32_t m_scoreBadCapture { ScoringOffsets::ScoreBadCapture };
    int32_t m_scoreGoodPromotion { ScoringOffsets::ScoreGoodPromotion };
    int32_t m_scoreBadPromotion { ScoringOffsets::ScoreBadPromotion };
    int32_t m_scoreKillerMove { ScoringOffsets::ScoreKillerMove };
    int32_t m_scoreHistoryMove { ScoringOffsets::ScoreHistoryMove };

    // For lazySMP, score can be adjusted by [-s_offsetRange, s_offsetRange]
    static uint8_t s_offsetRange;
};

uint8_t MoveOrdering::s_offsetRange { 20 };
}
