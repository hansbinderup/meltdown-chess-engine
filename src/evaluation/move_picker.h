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
    FIRST,
    SECOND,
};

enum PickerPhase {
    TT_MOVE,
    PV_MOVE,
    CAPTURE_GOOD,
    PROMOTION_GOOD,
    KILLER_MOVE_FIRST,
    KILLER_MOVE_SECOND,
    HISTORY_MOVE,
    PROMOTION_BAD,
    BAD_CAPTURE,
    DONE,
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

    template<Player player>
    constexpr std::optional<movegen::Move> pickNextMove(PickerPhase& phase, const BitBoard& board, movegen::ValidMoves& moves, uint8_t ply, const std::optional<movegen::Move>& ttMove = std::nullopt)
    {
        switch (phase) {
        case TT_MOVE: {
            const auto pickedMove = pickTtMove(moves, ttMove);

            if (pickedMove.has_value())
                return pickedMove;

            phase = PickerPhase::PV_MOVE;

            return pickNextMove<player>(phase, board, moves, ply, ttMove);
        }

        case PV_MOVE: {
            const auto pickedMove = pickPvMove(moves, ply);

            if (pickedMove.has_value()) {
                m_pvTable.setIsScoring(false);
                return pickedMove;
            }

            phase = PickerPhase::CAPTURE_GOOD;

            return pickNextMove<player>(phase, board, moves, ply, ttMove);
        }

        case CAPTURE_GOOD: {
            const auto pickedMove = pickCapture<true>(board, moves);

            if (pickedMove.has_value())
                return pickedMove;

            phase = PickerPhase::PROMOTION_GOOD;

            return pickNextMove<player>(phase, board, moves, ply, ttMove);
        }

        case PROMOTION_GOOD: {
            const auto pickedMove = pickPromotion<true>(moves);

            if (pickedMove.has_value())
                return pickedMove;

            phase = PickerPhase::KILLER_MOVE_FIRST;

            return pickNextMove<player>(phase, board, moves, ply, ttMove);
        }

        case KILLER_MOVE_FIRST: {
            const auto pickedMove = pickKillerMove(KillerMoveType::FIRST, moves, ply);

            if (pickedMove.has_value())
                return pickedMove;

            phase = PickerPhase::KILLER_MOVE_SECOND;

            return pickNextMove<player>(phase, board, moves, ply, ttMove);
        }

        case KILLER_MOVE_SECOND: {
            const auto pickedMove = pickKillerMove(KillerMoveType::SECOND, moves, ply);

            if (pickedMove.has_value())
                return pickedMove;

            phase = PickerPhase::HISTORY_MOVE;

            return pickNextMove<player>(phase, board, moves, ply, ttMove);
        }

        case HISTORY_MOVE: {
            const auto pickedMove = pickHistoryMove<player>(board, moves);

            if (pickedMove.has_value())
                return pickedMove;

            phase = PickerPhase::PROMOTION_BAD;

            return pickNextMove<player>(phase, board, moves, ply, ttMove);
        }

        case PROMOTION_BAD: {
            const auto pickedMove = pickPromotion<false>(moves);

            if (pickedMove.has_value())
                return pickedMove;

            phase = PickerPhase::BAD_CAPTURE;

            return pickNextMove<player>(phase, board, moves, ply, ttMove);
        }
        case BAD_CAPTURE: {
            const auto pickedMove = pickCapture<false>(board, moves);

            if (pickedMove.has_value()) {
                return pickedMove;
            }

            phase = PickerPhase::DONE;

            return pickNextMove<player>(phase, board, moves, ply, ttMove);
        }

        case DONE:
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
    constexpr std::optional<movegen::Move>
    pickTtMove(movegen::ValidMoves& moves, std::optional<movegen::Move> ttMove)
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

        const auto killerMove = type == KillerMoveType::FIRST ? killerMoves.first : killerMoves.second;

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
        int32_t bestScore = s_minScore;
        std::optional<movegen::Move> bestMove = std::nullopt;
        uint16_t bestMoveIndex {};

        for (uint16_t i = 0; i < moves.count(); i++) {
            if (!moves[i].isNull() && moves[i].isQuietMove()) {
                const auto attacker = board.getAttackerAtSquare<player>(moves[i].fromSquare());

                if (attacker.has_value()) {
                    const int32_t score = m_historyMoves.get(attacker.value(), moves[i].toPos());

                    if (score > bestScore) {
                        bestScore = score;
                        bestMove = moves[i];
                        bestMoveIndex = i;
                    }
                }
                // Debug only
                else {
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
