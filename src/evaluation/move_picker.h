#pragma once

#include "bit_board.h"

#include "evaluation/history_moves.h"
#include "evaluation/killer_moves.h"
#include "evaluation/pv_table.h"
#include "evaluation/see_swap.h"
#include "movegen/move_types.h"
#include <cstdint>

namespace evaluation {

enum KillerMoveType {
    First,
    Second,
};

enum PickerPhase {
    Syzygy,
    PvMove,
    TtMove,
    CaptureGood,
    PromotionGood,
    KillerMoveFirst,
    KillerMoveSecond,
    HistoryMove,
    PromotionBad,
    BadCapture,
    Done,
};

class MovePicker {
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

    template<Player player> constexpr std::optional<movegen::Move> pickNextMove(PickerPhase& phase, const BitBoard& board, movegen::ValidMoves& moves, uint8_t ply, std::optional<movegen::Move> ttMove = std::nullopt)
    {
        switch (phase) {
        case Syzygy: {

            if (const auto pickedMove = pickSyzygyMove(moves))
                return pickedMove;

            phase = PickerPhase::Done;

            return pickNextMove<player>(phase, board, moves, ply, ttMove);
        }

        case PvMove: {
            if (const auto pickedMove = pickPvMove(moves, ply))
                return pickedMove;

            phase = PickerPhase::TtMove;

            return pickNextMove<player>(phase, board, moves, ply, ttMove);
        }

        case TtMove: {
            if (const auto pickedMove = pickTtMove(moves, ttMove))
                return pickedMove;

            phase = PickerPhase::CaptureGood;

            return pickNextMove<player>(phase, board, moves, ply, ttMove);
        }

        case CaptureGood: {
            if (const auto pickedMove = pickCapture<true>(board, moves))
                return pickedMove;

            phase = PickerPhase::PromotionGood;

            return pickNextMove<player>(phase, board, moves, ply, ttMove);
        }

        case PromotionGood: {
            if (const auto pickedMove = pickPromotion<true>(moves))
                return pickedMove;

            phase = PickerPhase::KillerMoveFirst;

            return pickNextMove<player>(phase, board, moves, ply, ttMove);
        }

        case KillerMoveFirst: {
            if (const auto pickedMove = pickKillerMove(KillerMoveType::First, moves, ply))
                return pickedMove;

            phase = PickerPhase::KillerMoveSecond;

            return pickNextMove<player>(phase, board, moves, ply, ttMove);
        }

        case KillerMoveSecond: {
            if (const auto pickedMove = pickKillerMove(KillerMoveType::Second, moves, ply))
                return pickedMove;

            phase = PickerPhase::HistoryMove;

            return pickNextMove<player>(phase, board, moves, ply, ttMove);
        }

        case HistoryMove: {
            if (const auto pickedMove = pickHistoryMove<player>(board, moves))
                return pickedMove;

            phase = PickerPhase::PromotionBad;

            return pickNextMove<player>(phase, board, moves, ply, ttMove);
        }

        case PromotionBad: {
            if (const auto pickedMove = pickPromotion<false>(moves))
                return pickedMove;

            phase = PickerPhase::BadCapture;

            return pickNextMove<player>(phase, board, moves, ply, ttMove);
        }
        case BadCapture: {
            if (const auto pickedMove = pickCapture<false>(board, moves))
                return pickedMove;

            phase = PickerPhase::Done;

            return pickNextMove<player>(phase, board, moves, ply, ttMove);
        }

        case Done:
            break;
        }

        return std::nullopt;
    }

    // Helper: calling inside loops will mean redundant colour checks
    constexpr std::optional<movegen::Move>
    pickNextMove(PickerPhase& phase, const BitBoard& board, movegen::ValidMoves& moves, uint8_t ply, const std::optional<movegen::Move>& ttMove = std::nullopt)
    {
        if (board.player == PlayerWhite) {
            return pickNextMove<PlayerWhite>(phase, board, moves, ply, ttMove);
        } else {
            return pickNextMove<PlayerBlack>(phase, board, moves, ply, ttMove);
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

    constexpr std::optional<movegen::Move> pickTtMove(movegen::ValidMoves& moves, std::optional<movegen::Move> ttMove)
    {
        if (!ttMove.has_value())
            return std::nullopt;

        for (uint16_t i = 0; i < moves.count(); i++) {
            if (!moves[i].isNull() && moves[i] == *ttMove) {
                const auto ttMove = moves[i];

                moves.nullifyMove(i);

                return ttMove;
            }
        }
        return std::nullopt;
    }

    constexpr std::optional<movegen::Move> pickPvMove(movegen::ValidMoves& moves, uint8_t ply)
    {
        if (!m_pvTable.isScoring())
            return std::nullopt;

        for (uint16_t i = 0; i < moves.count(); i++) {
            if (!moves[i].isNull() && m_pvTable.isPvMove(moves[i], ply)) {
                const auto pickedMove = moves[i];

                moves.nullifyMove(i);

                m_pvTable.setIsScoring(false);

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

    constexpr std::optional<movegen::Move> pickKillerMove(KillerMoveType type, movegen::ValidMoves& moves, uint8_t ply)
    {
        const auto killerMoves = m_killerMoves.get(ply);

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

    template<Player player>
    constexpr std::optional<movegen::Move> pickHistoryMove(const BitBoard& board, movegen::ValidMoves& moves)
    {
        int32_t bestScore = std::numeric_limits<int32_t>::min();
        std::optional<movegen::Move> bestMove = std::nullopt;
        uint16_t bestMoveIndex {};

        for (uint16_t i = 0; i < moves.count(); i++) {
            if (!moves[i].isNull() && moves[i].isQuietMove()) {
                if (const auto attacker = board.getAttackerAtSquare<player>(moves[i].fromSquare())) {
                    const int32_t score = m_historyMoves.get(attacker.value(), moves[i].toPos());

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

    KillerMoves m_killerMoves {};
    HistoryMoves m_historyMoves {};

    PVTable m_pvTable {};
};
}
