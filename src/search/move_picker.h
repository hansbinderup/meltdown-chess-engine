#pragma once

#include "core/bit_board.h"

#include "evaluation/see_swap.h"
#include "movegen/move_types.h"

#include "search/search_tables.h"

#include <cstdint>

namespace search {

enum KillerMoveType {
    First,
    Second,
};

enum PickerPhase {
    Syzygy,
    TtMove,
    PvMove,
    CaptureGood,
    PromotionGood,
    KillerMoveFirst,
    KillerMoveSecond,
    CounterMove,
    HistoryMove,
    PromotionBad,
    BadCapture,
    Done,
};

class MovePicker {
public:
    MovePicker(
        SearchTables& searchTables, uint8_t ply, std::optional<movegen::Move> ttMove = std::nullopt, std::optional<movegen::Move> prevMove = std::nullopt, PickerPhase phase = PickerPhase::TtMove)
        : m_searchTables(searchTables)
        , m_ply(ply)
        , m_ttMove(ttMove)
        , m_prevMove(prevMove)
        , m_phase(phase)
    {
    }

    void setPhase(PickerPhase phase)
    {
        m_phase = phase;
    }

    template<Player player> constexpr std::optional<movegen::Move> pickNextMove(const BitBoard& board, movegen::ValidMoves& moves)
    {
        switch (m_phase) {
        case Syzygy: {

            if (const auto pickedMove = pickSyzygyMove(moves))
                return pickedMove;

            m_phase = PickerPhase::Done;

            return pickNextMove<player>(board, moves);
        }

        case TtMove: {
            if (const auto pickedMove = pickTtMove(moves))
                return pickedMove;

            m_phase = PickerPhase::PvMove;

            return pickNextMove<player>(board, moves);
        }

        case PvMove: {
            if (const auto pickedMove = pickPvMove(moves))
                return pickedMove;

            m_phase = PickerPhase::CaptureGood;

            return pickNextMove<player>(board, moves);
        }

        case CaptureGood: {
            if (const auto pickedMove = pickCapture<true>(board, moves))
                return pickedMove;

            m_phase = PickerPhase::PromotionGood;

            return pickNextMove<player>(board, moves);
        }

        case PromotionGood: {
            if (const auto pickedMove = pickPromotion<true>(moves))
                return pickedMove;

            m_phase = PickerPhase::KillerMoveFirst;

            return pickNextMove<player>(board, moves);
        }

        case KillerMoveFirst: {
            if (const auto pickedMove = pickKillerMove(KillerMoveType::First, moves))
                return pickedMove;

            m_phase = PickerPhase::KillerMoveSecond;

            return pickNextMove<player>(board, moves);
        }

        case KillerMoveSecond: {
            if (const auto pickedMove = pickKillerMove(KillerMoveType::Second, moves))
                return pickedMove;

            m_phase = PickerPhase::CounterMove;

            return pickNextMove<player>(board, moves);
        }

        case CounterMove: {
            if (const auto pickedMove = pickCounterMove(moves))
                return pickedMove;

            m_phase = PickerPhase::HistoryMove;

            return pickNextMove<player>(board, moves);
        }

        case HistoryMove: {
            if (const auto pickedMove = pickHistoryMove<player>(board, moves))
                return pickedMove;

            m_phase = PickerPhase::PromotionBad;

            return pickNextMove<player>(board, moves);
        }

        case PromotionBad: {
            if (const auto pickedMove = pickPromotion<false>(moves))
                return pickedMove;

            m_phase = PickerPhase::BadCapture;

            return pickNextMove<player>(board, moves);
        }
        case BadCapture: {
            if (const auto pickedMove = pickCapture<false>(board, moves))
                return pickedMove;

            m_phase = PickerPhase::Done;

            return pickNextMove<player>(board, moves);
        }

        case Done:
            break;
        }

        return std::nullopt;
    }

    // Helper: calling inside loops will mean redundant colour checks
    constexpr std::optional<movegen::Move> pickNextMove(const BitBoard& board, movegen::ValidMoves& moves)
    {
        if (board.player == PlayerWhite) {
            return pickNextMove<PlayerWhite>(board, moves);
        } else {
            return pickNextMove<PlayerBlack>(board, moves);
        }
    }

private:
    // Syzygy moves are already sorted: return first, then second, third etc
    constexpr std::optional<movegen::Move> pickSyzygyMove(movegen::ValidMoves& moves)
    {
        for (uint16_t i = 0; i < moves.count(); i++) {
            if (!moves[i].isNull()) {
                const auto pickedMove = moves[i];

                moves.nullifyMove(i);

                return pickedMove;
            }
        }
        return std::nullopt;
    }

    constexpr std::optional<movegen::Move> pickTtMove(movegen::ValidMoves& moves)
    {
        if (!m_ttMove.has_value())
            return std::nullopt;

        for (uint16_t i = 0; i < moves.count(); i++) {
            if (!moves[i].isNull() && moves[i] == *m_ttMove) {
                const auto ttMove = moves[i];

                moves.nullifyMove(i);

                return ttMove;
            }
        }
        return std::nullopt;
    }

    constexpr std::optional<movegen::Move> pickPvMove(movegen::ValidMoves& moves)
    {
        if (!m_searchTables.isPvScoring())
            return std::nullopt;

        for (uint16_t i = 0; i < moves.count(); i++) {
            if (!moves[i].isNull() && m_searchTables.isPvMove(moves[i], m_ply)) {
                const auto pickedMove = moves[i];

                moves.nullifyMove(i);

                m_searchTables.setPvIsScoring(false);

                return pickedMove;
            }
        }
        return std::nullopt;
    }

    template<bool isGood>
    constexpr std::optional<movegen::Move> pickCapture(const BitBoard& board, movegen::ValidMoves& moves)
    {
        std::optional<movegen::Move> bestMove { std::nullopt };
        int32_t bestScore = s_minScore;
        uint16_t bestMoveIndex {};

        for (uint16_t i = 0; i < moves.count(); i++) {
            if (!moves[i].isNull() && moves[i].isCapture()) {
                int32_t seeScore = evaluation::SeeSwap::run(board, moves[i]);

                if constexpr (isGood) {
                    if (seeScore >= 0 && seeScore > bestScore) {
                        bestMove = moves[i];
                        bestScore = seeScore;
                        bestMoveIndex = i;
                    }
                } else {
                    // Already eliminated good captures
                    if (seeScore > bestScore) {
                        bestMove = moves[i];
                        bestScore = seeScore;
                        bestMoveIndex = i;
                    }
                }
            }
        }
        if (bestMove.has_value())
            moves.nullifyMove(bestMoveIndex);

        return bestMove;
    }

    template<bool isGood>
    constexpr std::optional<movegen::Move> pickPromotion(movegen::ValidMoves& moves)
    {
        if constexpr (isGood) {
            /* Currently, good captures are considered better than good promotions, and bad captures are worse than bad promotions */
            for (uint16_t i = 0; i < moves.count(); i++) {
                if (!moves[i].isNull() && moves[i].promotionType() == PromotionQueen && !moves[i].isCapture()) {
                    const auto pickedMove = moves[i];

                    moves.nullifyMove(i);

                    return pickedMove;
                }
            }
        } else {
            for (uint16_t i = 0; i < moves.count(); i++) {
                if (!moves[i].isNull() && moves[i].isPromotionMove() && !moves[i].isCapture()) {
                    const auto pickedMove = moves[i];

                    moves.nullifyMove(i);

                    return pickedMove;
                }
            }
        }

        return std::nullopt;
    }

    constexpr std::optional<movegen::Move> pickKillerMove(KillerMoveType type, movegen::ValidMoves& moves)
    {
        const auto killerMoves = m_searchTables.getKillerMove(m_ply);

        const auto killerMove = type == KillerMoveType::First ? killerMoves.first : killerMoves.second;

        for (uint16_t i = 0; i < moves.count(); i++) {

            if (!moves[i].isNull() && moves[i] == killerMove && moves[i].isQuietMove()) {
                auto pickedMove = moves[i];

                moves.nullifyMove(i);

                return pickedMove;
            }
        }
        return std::nullopt;
    }

    constexpr std::optional<movegen::Move> pickCounterMove(movegen::ValidMoves& moves)
    {
        if (!m_prevMove.has_value())
            return std::nullopt;

        const auto counterMove = m_searchTables.getCounterMove(m_prevMove.value());

        for (uint16_t i = 0; i < moves.count(); i++) {
            if (!moves[i].isNull() && moves[i].isQuietMove() && moves[i] == counterMove) {
                auto pickedMove = moves[i];

                moves.nullifyMove(i);

                return pickedMove;
            }
        }
        return std::nullopt;
    }

    template<Player player>
    constexpr std::optional<movegen::Move> pickHistoryMove(const BitBoard& board, movegen::ValidMoves& moves)
    {
        int32_t bestScore = std::numeric_limits<int32_t>::min();
        std::optional<movegen::Move> bestMove = std::nullopt;
        uint16_t bestMoveIndex {};

        for (uint16_t i = 0; i < moves.count(); i++) {
            if (!moves[i].isNull() && moves[i].isQuietMove()) {
                if (const auto attacker = board.getAttackerAtSquare<player>(moves[i].fromSquare())) {
                    const int32_t score = m_searchTables.getHistoryMove(attacker.value(), moves[i].toPos());

                    if (score > bestScore) {
                        bestScore = score;
                        bestMove = moves[i];
                        bestMoveIndex = i;
                    }
                } else {
                    assert(false);
                }
            }
        }
        if (bestMove.has_value())
            moves.nullifyMove(bestMoveIndex);

        return bestMove;
    }

    SearchTables& m_searchTables;
    uint8_t m_ply {};
    std::optional<movegen::Move> m_ttMove { std::nullopt };
    std::optional<movegen::Move> m_prevMove { std::nullopt };
    PickerPhase m_phase { PickerPhase::TtMove };
};
}
