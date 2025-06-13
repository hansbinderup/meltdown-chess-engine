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

enum PromotionOffsets : int32_t {
    Good = 8000,
    Bad = -8000,
};

enum PickerPhase {
    GenerateSyzygyMoves,
    Syzygy,
    GenerateMoves,
    TtMove,
    PvMove,
    ScoreTacticals,
    TacticalGood,
    KillerMoveFirst,
    KillerMoveSecond,
    CounterMove,
    ScoreQuiets,
    QuietMove,
    TacticalBad,
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
        m_scores.fill(std::numeric_limits<int32_t>::min());

        if constexpr (moveType == movegen::MoveCapture) {
            if (m_ttMove.has_value() && !m_ttMove->isCapture()) {
                m_ttMove.reset();
            }
        }

        m_isFollowingPvLine = m_searchTables.isPvFollowing();
    }

    constexpr uint16_t numGeneratedMoves()
    {
        return m_ttMove ? 1 : m_moves.count();
    }

    template<Player player> constexpr std::optional<movegen::Move> pickNextMove(const BitBoard& board)
    {
        switch (m_phase) {
        case GenerateSyzygyMoves: {
            const bool syzygyActive = syzygy::generateSyzygyMoves(board, m_moves);

            if (syzygyActive) {
                m_phase = PickerPhase::Syzygy;
            } else {
                m_phase = PickerPhase::TtMove;
            }

            return pickNextMove<player>(board);
        }

        case Syzygy: {
            if (const auto pickedMove = pickSyzygyMove())
                return pickedMove;

            m_phase = PickerPhase::Done;

            return pickNextMove<player>(board);
        }

        case TtMove: {
            m_phase = PickerPhase::GenerateMoves;

            if (const auto pickedMove = pickTtMove()) {
                if (m_isFollowingPvLine) {
                    m_searchTables.updatePvScoring(*pickedMove, m_ply);
                }

                return pickedMove;
            }

            return pickNextMove<player>(board);
        }

        case GenerateMoves: {
            generateAllMoves(board);

            m_phase = PickerPhase::PvMove;

            return pickNextMove<player>(board);
        }

        case PvMove: {
            if (const auto pickedMove = pickPvMove())
                return pickedMove;

            m_phase = PickerPhase::ScoreTacticals;

            return pickNextMove<player>(board);
        }

        case ScoreTacticals: {
            generateTacticalScores(board);

            m_phase = PickerPhase::TacticalGood;

            return pickNextMove<player>(board);
        }

        case TacticalGood: {
            if (const auto pickedMove = pickTacticalMove<true>())
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

            m_phase = PickerPhase::ScoreQuiets;

            return pickNextMove<player>(board);
        }

        case ScoreQuiets: {
            generateQuietScores<player>(board);

            m_phase = PickerPhase::QuietMove;

            return pickNextMove<player>(board);
        }

        case QuietMove: {
            if (const auto pickedMove = pickQuietMove())
                return pickedMove;

            m_phase = PickerPhase::TacticalBad;

            return pickNextMove<player>(board);
        }

        case TacticalBad: {
            if (const auto pickedMove = pickTacticalMove<false>())
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

        if (m_isFollowingPvLine) {
            m_searchTables.updatePvScoring(m_moves, m_ply);
        }

        if (!m_ttMove.has_value())
            return;

        for (uint16_t i = 0; i < m_moves.count(); i++) {
            if (m_moves[i] == m_ttMove.value()) {
                m_moves.nullifyMove(i);
                m_ttMove.reset();

                break;
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
        return m_ttMove;
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

    constexpr void generateTacticalScores(const BitBoard& board)
    {
        for (uint16_t i = 0; i < m_moves.count(); i++) {
            // If it's a promotion, assign promotion score
            if (m_moves[i].promotionType() == PromotionQueen)
                m_scores[i] = PromotionOffsets::Good;
            else if (m_moves[i].isPromotionMove())
                m_scores[i] = PromotionOffsets::Bad;
            else if (m_moves[i].isCapture())
                m_scores[i] = evaluation::SeeSwap::run(board, m_moves[i]);
        }
    }

    template<bool isGood>
    constexpr std::optional<movegen::Move> pickTacticalMove()
    {
        std::optional<movegen::Move> bestMove { std::nullopt };
        int32_t bestScore = std::numeric_limits<int32_t>::min();
        uint16_t bestMoveIndex {};

        for (uint16_t i = 0; i < m_moves.count(); i++) {
            if constexpr (isGood) {
                if (m_moves[i].promotionType() == PromotionQueen || m_moves[i].isCapture()) {

                    if (m_scores[i] < 0)
                        continue;

                    if (m_scores[i] > bestScore) {
                        bestMove = m_moves[i];
                        bestScore = m_scores[i];
                        bestMoveIndex = i;
                    }
                }

            }
            // Already picked all good promotions and captures
            else {
                if (m_moves[i].isPromotionMove() || m_moves[i].isCapture()) {

                    if (m_scores[i] > bestScore) {
                        bestMove = m_moves[i];
                        bestScore = m_scores[i];
                        bestMoveIndex = i;
                    }
                }
            }
        }

        if (bestMove.has_value()) {
            m_moves.nullifyMove(bestMoveIndex);
            m_scores[bestMoveIndex] = std::numeric_limits<int32_t>::min();
        }

        return bestMove;
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
    constexpr void generateQuietScores(const BitBoard& board)
    {

        for (uint16_t i = 0; i < m_moves.count(); i++) {
            if (!m_moves[i].isNull() && m_moves[i].isQuietMove()) {

                const auto attacker = board.getAttackerAtSquare<player>(m_moves[i].fromSquare());
                m_scores[i] = m_searchTables.getHistoryMove(attacker.value(), m_moves[i].toPos());
            }
        }
    }

    constexpr std::optional<movegen::Move> pickQuietMove()
    {
        int32_t bestScore = std::numeric_limits<int32_t>::min();
        std::optional<movegen::Move> bestMove = std::nullopt;
        uint16_t bestMoveIndex {};

        for (uint16_t i = 0; i < m_moves.count(); i++) {
            if (!m_moves[i].isNull() && m_moves[i].isQuietMove()) {
                if (m_scores[i] > bestScore) {
                    bestScore = m_scores[i];
                    bestMove = m_moves[i];
                    bestMoveIndex = i;
                }
            }
        }
        if (bestMove.has_value())
            m_moves.nullifyMove(bestMoveIndex);
        m_scores[bestMoveIndex] = std::numeric_limits<int32_t>::min();

        return bestMove;
    }

    SearchTables& m_searchTables;

    uint8_t m_ply {};
    PickerPhase m_phase { PickerPhase::TtMove };
    std::optional<movegen::Move> m_ttMove { std::nullopt };
    std::optional<movegen::Move> m_prevMove { std::nullopt };

    movegen::ValidMoves m_moves {};
    // Sorting scores for the moves
    std::array<int32_t, s_maxMoves> m_scores {};

    bool m_isFollowingPvLine { false };
};
}
