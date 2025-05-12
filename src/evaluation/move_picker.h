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

enum CaptureType {
    GOOD,
    BAD,
};

enum KillerMoveType {
    FIRST,
    SECOND,
};

enum PickerPhase {
    TT_MOVE,
    PV_MOVE,
    GOOD_CAPTURE,
    BAD_CAPTURE,
    PROMOTION_QUEEN,
    PROMOTION_ROOK,
    PROMOTION_KNIGHT,
    PROMOTION_BISHOP,
    KILLER_MOVE_FIRST,
    KILLER_MOVE_SECOND,
    HISTORY_MOVE,
    QUIET_MOVE,
    DONE,
};

class MovePicker {
public:
    constexpr void setMoves(movegen::ValidMoves moves)
    {
        m_moves = moves;
    }

    template<Player player>
    constexpr std::optional<movegen::Move> pickNextMove(const BitBoard& board, uint8_t ply, std::optional<movegen::Move> ttMove = std::nullopt)
    {
        switch (m_phase) {
        case TT_MOVE: {
            const auto pickedMove = pickTtMove(ttMove);

            if (pickedMove.has_value())
                return pickedMove;

            m_phase = PickerPhase::PV_MOVE;

            return selectNextMove<player>(board, m_moves, ply, ttMove);
        }

        case PV_MOVE: {
            const auto pickedMove = pickPvMove(ply);

            if (pickedMove.has_value())
                return pickedMove;

            m_phase = PickerPhase::GOOD_CAPTURE;

            return selectNextMove<player>(board, m_moves, ply, ttMove);
        }

        case GOOD_CAPTURE: {
            const auto pickedMove = pickCapture(CaptureType::GOOD, board);
            if (pickedMove.has_value())
                return pickedMove;

            m_head = 0;
            m_phase = PickerPhase::BAD_CAPTURE;

            return selectNextMove<player>(board, m_moves, ply, ttMove);
        }

        case BAD_CAPTURE: {
            const pickedMove = pickCapture(CaptureType::BAD, board);
            if (pickedMove.has_value())
                return pickedMove;

            m_head = 0;
            m_phase = PickerPhase::PROMOTION_QUEEN;

            return selectNextMove<player>(board, m_moves, ply, ttMove);
        }

        case PROMOTION_QUEEN: {
            const auto pickedMove = pickPromotion(PromotionQueen);
            if (pickedMove.has_value())
                return pickedMove;

            m_phase = PickerPhase::PROMOTION_ROOK;

            return selectNextMove<player>(board, m_moves, ply, ttMove);
        }

        case PROMOTION_ROOK: {
            const auto pickedMove = pickPromotion(PromotionRook);
            if (pickedMove.has_value())
                return pickedMove;

            m_phase = PickerPhase::PROMOTION_KNIGHT;

            return selectNextMove<player>(board, m_moves, ply, ttMove);
        }

        case PROMOTION_KNIGHT: {
            const auto pickedMove = pickPromotion(PromotionKnight);
            if (pickedMove.has_value())
                return pickedMove;

            m_phase = PickerPhase::PROMOTION_BISHOP;

            return selectNextMove<player>(board, m_moves, ply, ttMove);
        }

        case PROMOTION_BISHOP: {
            const auto pickedMove = pickPromotion(PromotionBishop);
            if (pickedMove.has_value())
                return pickedMove;

            m_phase = PickerPhase::KILLER_MOVE_FIRST;

            return selectNextMove<player>(board, m_moves, ply, ttMove);
        }

        case KILLER_MOVE_FIRST: {
            const auto pickedMove = pickKillerMove(KillerMoveType::FIRST, ply);

            if (pickedMove.has_value())
                return pickedMove;

            m_phase = PickerPhase::KILLER_MOVE_SECOND;

            return selectNextMove<player>(board, m_moves, ply, ttMove);
        }

        case KILLER_MOVE_SECOND: {
            const auto pickedMove = pickKillerMove(KillerMoveType::FIRST, ply);

            if (pickedMove.has_value())
                return pickedMove;

            m_phase = PickerPhase::HISTORY_MOVE;

            return selectNextMove<player>(board, m_moves, ply, ttMove);
        }

        case HISTORY_MOVE: {
            const auto pickedMove = pickHistoryMove<player>(board);

            if (pickedMove.has_value())
                return pickedMove;

            m_phase = PickerPhase::QUIET_MOVE;

            return selectNextMove<player>(board, m_moves, ply, ttMove);
        }

        case QUIET_MOVE: {
            const auto pickedMove = pickHistoryMove<player>(board);

            if (pickedMove.has_value())
                return pickedMove;

            m_phase = PickerPhase::DONE;
            return selectNextMove<player>(board, m_moves, ply, ttMove);
        }

        case DONE:
            // TODO. Reset everything and return something nullopt-like
            return std::nullopt;
        }
    }

private:
    constexpr std::optional<movegen::Move>
    pickTtMove(std::optional<movegen::Move> ttMove)
    {

        for (uint8_t i = 0; i < m_moves.count(); i++) {
            if (ttMove.has_value() && m_moves[i] == *ttMove) {
                const auto move = m_moves[i];

                m_moves.nullifyMove(i);

                return move;
            }
        }
        return std::nullopt;
    }

    constexpr std::optional<movegen::Move> pickPvMove(uint8_t ply)
    {
        for (uint8_t i = 0; i < m_moves.count(); i++) {
            if (m_pvTable.isScoring() && m_pvTable.isPvMove(m_moves[i], ply)) {
                const auto pickedMove = m_moves[i];

                m_moves.nullifyMove(i);
                m_phase = PickerPhase::GOOD_CAPTURE;

                return pickedMove;
            }
        }
        return std::nullopt;
    }

    constexpr std::optional<movegen::Move> pickCapture(CaptureType type, const BitBoard& board)
    {

        for (uint8_t i = m_head; i < m_moves.count(); i++) {
            if (m_moves[i].isCapture()) {
                int32_t seeScore = evaluation::SeeSwap::run(board, m_moves[i]);

                if ((seeScore >= 0 && type == CaptureType::GOOD) || (seeScore < 0 && type == CaptureType::BAD)) {
                    auto move = m_moves[i];

                    m_head = i + 1;
                    m_moves.nullifyMove(i);

                    return move;
                }
            }
        }
        return std::nullopt;
    }

    constexpr std::optional<movegen::Move> pickPromotion(PromotionType type)
    {

        for (uint8_t i = 0; i < m_moves.count(); i++) {
            if (m_moves[i].promotionType() == type) {
                const auto move = m_moves[i];

                m_moves.nullifyMove(i);

                return move;
            }
        }
        return std::nullopt;
    }

    constexpr std::optional<movegen::Move> pickKillerMove(KillerMoveType type, uint8_t ply)
    {
        const auto killerMoves = m_killerMoves.get(ply);

        for (uint8_t i = 0; i < m_moves.count(); i++) {
            const auto killerMove = type == KillerMoveType::FIRST ? killerMoves.first : killerMoves.second;
            if (m_moves[i] == killerMove) {
                auto move = m_moves[i];

                m_moves.nullifyMove(i);

                return move;
            }
        }
        return std::nullopt;
    }

    template<Player player>
    constexpr std::optional<movegen::Move> pickHistoryMove(const BitBoard& board)
    {
        if (!isSortedHistoryPicks) {
            // Sorting
            // TODO change const auto& Move to move everywhere
            for (uint8_t i = 0; i < m_moves.count(); i++) {
                const auto attacker = board.getAttackerAtSquare<player>(m_moves[i].fromSquare());
                if (attacker.has_value()) {
                    const int32_t score = m_historyMoves.get(attacker.value(), m_moves[i].toPos());

                    m_historyPicks.at(m_historyPicksHead) = { m_moves[i], score };
                    ++m_historyPicksSize;

                    m_moves.nullifyMove(i);
                }
            }

            // TODO move to another function?
            for (size_t i = 1; i < m_historyPicksSize; i++) {
                const auto& [key, score] = m_historyPicks[i];

                size_t j = i;
                while (j > 0 && m_historyPicks[j - 1].second < score) {
                    m_historyPicks[j] = m_historyPicks[j - 1];
                    --j;
                }
                m_historyPicks[j] = { key, score };
            }

            isSortedHistoryPicks = true;
        }

        // Returning
        if (m_historyPicksHead != m_historyPicksSize) {
            const auto move = m_historyPicks[m_historyPicksHead].first;

            ++m_historyPicksHead;

            return move;
        }
        isSortedHistoryPicks = false;
        m_historyPicks = std::array<std::pair<movegen::Move, int32_t>, s_maxMoves>();
        m_historyPicksSize = 0;
        m_historyPicksHead = 0;

        return std::nullopt;
    }

    constexpr std::optional<movegen::Move> PickQuietMove()
    {
        for (uint8_t i = m_head; i < m_moves.count(); i++) {
            if (!m_moves[i].isNull()) {
                const auto move = m_moves[i];

                m_moves.nullifyMove(i);
                return move;
            }
        }
        return std::nullopt;
    }

    // TODO uint16_t for move counting?

    PickerPhase m_phase {};

    movegen::ValidMoves m_moves {};
    uint8_t m_head {};

    std::array<std::pair<movegen::Move, int32_t>, s_maxMoves> m_historyPicks {};
    bool isSortedHistoryPicks { false };
    uint8_t m_historyPicksHead {};
    uint8_t m_historyPicksSize {};

    KillerMoves m_killerMoves {};
    HistoryMoves m_historyMoves {};

    PVTable m_pvTable {};
};
}
