#pragma once

#include "core/bit_board.h"
#include "core/move_handling.h"

#include "evaluation/see_swap.h"
#include "movegen/move_types.h"

#include "search/search_tables.h"
#include "syzygy/syzygy.h"

#include <cstdint>

namespace search {

enum KillerMoveType {
    First,
    Second,
};

enum PickerPhase {
    GenerateSyzygyMoves,
    Syzygy,
    GenerateMoves,
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

template<movegen::MoveType moveType>
class MovePicker {
public:
    MovePicker(SearchTables& searchTables, uint8_t ply, PickerPhase phase, std::optional<movegen::Move> ttMove = std::nullopt, std::optional<movegen::Move> prevMove = std::nullopt)
        : m_searchTables(searchTables)
        , m_ply(ply)
        , m_phase(phase)
        , m_ttMove(ttMove)
        , m_prevMove(prevMove)
    {
    }

    constexpr uint16_t numGeneratedMoves()
    {
        return m_moves.count();
    }

    template<Player player> constexpr std::optional<movegen::Move> pickNextMove(const BitBoard& board)
    {
        switch (m_phase) {
        case GenerateSyzygyMoves: {
            const bool syzygyActive = syzygy::generateSyzygyMoves(board, m_moves);

            if (syzygyActive) {
                m_phase = PickerPhase::Syzygy;
            } else {
                m_phase = PickerPhase::GenerateMoves;
            }

            return pickNextMove<player>(board);
        }

        case Syzygy: {

            if (const auto pickedMove = pickSyzygyMove())
                return pickedMove;

            m_phase = PickerPhase::Done;

            return pickNextMove<player>(board);
        }

        case GenerateMoves: {
            generateAllMoves(board);

            m_phase = PickerPhase::TtMove;

            return pickNextMove<player>(board);
        }

        case TtMove: {
            if (const auto pickedMove = pickTtMove())
                return pickedMove;

            m_phase = PickerPhase::PvMove;

            return pickNextMove<player>(board);
        }

        case PvMove: {
            if (const auto pickedMove = pickPvMove())
                return pickedMove;

            m_phase = PickerPhase::CaptureGood;

            return pickNextMove<player>(board);
        }

        case CaptureGood: {
            if (const auto pickedMove = pickCapture<true>(board))
                return pickedMove;

            m_phase = PickerPhase::PromotionGood;

            return pickNextMove<player>(board);
        }

        case PromotionGood: {
            if (const auto pickedMove = pickPromotion<true>())
                return pickedMove;

            m_phase = PickerPhase::KillerMoveFirst;

            return pickNextMove<player>(board);
        }

        case KillerMoveFirst: {
            if (const auto pickedMove = pickKillerMove(KillerMoveType::First))
                return pickedMove;

            m_phase = PickerPhase::KillerMoveSecond;

            return pickNextMove<player>(board);
        }

        case KillerMoveSecond: {
            if (const auto pickedMove = pickKillerMove(KillerMoveType::Second))
                return pickedMove;

            m_phase = PickerPhase::CounterMove;

            return pickNextMove<player>(board);
        }

        case CounterMove: {
            if (const auto pickedMove = pickCounterMove())
                return pickedMove;

            m_phase = PickerPhase::HistoryMove;

            return pickNextMove<player>(board);
        }

        case HistoryMove: {
            if (const auto pickedMove = pickHistoryMove<player>(board))
                return pickedMove;

            m_phase = PickerPhase::PromotionBad;

            return pickNextMove<player>(board);
        }

        case PromotionBad: {
            if (const auto pickedMove = pickPromotion<false>())
                return pickedMove;

            m_phase = PickerPhase::BadCapture;

            return pickNextMove<player>(board);
        }
        case BadCapture: {
            if (const auto pickedMove = pickCapture<false>(board))
                return pickedMove;

            m_phase = PickerPhase::Done;

            return pickNextMove<player>(board);
        }

        case Done:
            break;
        }

        return std::nullopt;
    }

    // Helper: calling inside loops will mean redundant colour checks
    constexpr std::optional<movegen::Move> pickNextMove(const BitBoard& board)
    {
        if (board.player == PlayerWhite) {
            return pickNextMove<PlayerWhite>(board);
        } else {
            return pickNextMove<PlayerBlack>(board);
        }
    }

private:
    constexpr void generateAllMoves(const BitBoard& board)
    {
        core::getAllMoves<moveType>(board, m_moves);

        if constexpr (moveType == movegen::MovePseudoLegal) {
            if (m_searchTables.isPvFollowing()) {
                m_searchTables.updatePvScoring(m_moves, m_ply);
            }
        }
    }

    // Syzygy moves are already sorted: return first, then second, third etc
    constexpr std::optional<movegen::Move> pickSyzygyMove()
    {
        for (uint16_t i = 0; i < m_moves.count(); i++) {
            if (!m_moves[i].isNull()) {
                const auto pickedMove = m_moves[i];

                m_moves.nullifyMove(i);

                return pickedMove;
            }
        }
        return std::nullopt;
    }

    constexpr std::optional<movegen::Move> pickTtMove()
    {
        if (!m_ttMove.has_value())
            return std::nullopt;

        for (uint16_t i = 0; i < m_moves.count(); i++) {
            if (!m_moves[i].isNull() && m_moves[i] == *m_ttMove) {
                const auto ttMove = m_moves[i];

                m_moves.nullifyMove(i);

                return ttMove;
            }
        }
        return std::nullopt;
    }

    constexpr std::optional<movegen::Move> pickPvMove()
    {
        if (!m_searchTables.isPvScoring())
            return std::nullopt;

        for (uint16_t i = 0; i < m_moves.count(); i++) {
            if (!m_moves[i].isNull() && m_searchTables.isPvMove(m_moves[i], m_ply)) {
                const auto pickedMove = m_moves[i];

                m_moves.nullifyMove(i);

                m_searchTables.setPvIsScoring(false);

                return pickedMove;
            }
        }
        return std::nullopt;
    }

    template<bool isGood>
    constexpr std::optional<movegen::Move> pickCapture(const BitBoard& board)
    {
        std::optional<movegen::Move> bestMove { std::nullopt };
        int32_t bestScore = s_minScore;
        uint16_t bestMoveIndex {};

        for (uint16_t i = 0; i < m_moves.count(); i++) {
            if (!m_moves[i].isNull() && m_moves[i].isCapture()) {
                int32_t seeScore = evaluation::SeeSwap::run(board, m_moves[i]);

                if constexpr (isGood) {
                    if (seeScore >= 0 && seeScore > bestScore) {
                        bestMove = m_moves[i];
                        bestScore = seeScore;
                        bestMoveIndex = i;
                    }
                } else {
                    // Already eliminated good captures
                    if (seeScore > bestScore) {
                        bestMove = m_moves[i];
                        bestScore = seeScore;
                        bestMoveIndex = i;
                    }
                }
            }
        }
        if (bestMove.has_value())
            m_moves.nullifyMove(bestMoveIndex);

        return bestMove;
    }

    template<bool isGood>
    constexpr std::optional<movegen::Move> pickPromotion()
    {
        if constexpr (isGood) {
            /* Currently, good captures are considered better than good promotions, and bad captures are worse than bad promotions */
            for (uint16_t i = 0; i < m_moves.count(); i++) {
                if (!m_moves[i].isNull() && m_moves[i].promotionType() == PromotionQueen && !m_moves[i].isCapture()) {
                    const auto pickedMove = m_moves[i];

                    m_moves.nullifyMove(i);

                    return pickedMove;
                }
            }
        } else {
            for (uint16_t i = 0; i < m_moves.count(); i++) {
                if (!m_moves[i].isNull() && m_moves[i].isPromotionMove() && !m_moves[i].isCapture()) {
                    const auto pickedMove = m_moves[i];

                    m_moves.nullifyMove(i);

                    return pickedMove;
                }
            }
        }

        return std::nullopt;
    }

    constexpr std::optional<movegen::Move> pickKillerMove(KillerMoveType type)
    {
        const auto killerMoves = m_searchTables.getKillerMove(m_ply);

        const auto killerMove = type == KillerMoveType::First ? killerMoves.first : killerMoves.second;

        for (uint16_t i = 0; i < m_moves.count(); i++) {

            if (!m_moves[i].isNull() && m_moves[i] == killerMove && m_moves[i].isQuietMove()) {
                auto pickedMove = m_moves[i];

                m_moves.nullifyMove(i);

                return pickedMove;
            }
        }
        return std::nullopt;
    }

    constexpr std::optional<movegen::Move> pickCounterMove()
    {
        if (!m_prevMove.has_value())
            return std::nullopt;

        const auto counterMove = m_searchTables.getCounterMove(m_prevMove.value());

        for (uint16_t i = 0; i < m_moves.count(); i++) {
            if (!m_moves[i].isNull() && m_moves[i].isQuietMove() && m_moves[i] == counterMove) {
                auto pickedMove = m_moves[i];

                m_moves.nullifyMove(i);

                return pickedMove;
            }
        }
        return std::nullopt;
    }

    template<Player player>
    constexpr std::optional<movegen::Move> pickHistoryMove(const BitBoard& board)
    {
        int32_t bestScore = std::numeric_limits<int32_t>::min();
        std::optional<movegen::Move> bestMove = std::nullopt;
        uint16_t bestMoveIndex {};

        for (uint16_t i = 0; i < m_moves.count(); i++) {
            if (!m_moves[i].isNull() && m_moves[i].isQuietMove()) {
                if (const auto attacker = board.getAttackerAtSquare<player>(m_moves[i].fromSquare())) {
                    const int32_t score = m_searchTables.getHistoryMove(attacker.value(), m_moves[i].toPos());

                    if (score > bestScore) {
                        bestScore = score;
                        bestMove = m_moves[i];
                        bestMoveIndex = i;
                    }
                } else {
                    assert(false);
                }
            }
        }
        if (bestMove.has_value())
            m_moves.nullifyMove(bestMoveIndex);

        return bestMove;
    }

    SearchTables& m_searchTables;

    uint8_t m_ply {};
    PickerPhase m_phase { PickerPhase::TtMove };
    std::optional<movegen::Move> m_ttMove { std::nullopt };
    std::optional<movegen::Move> m_prevMove { std::nullopt };

    movegen::ValidMoves m_moves {};
};
}
