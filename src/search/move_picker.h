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

enum MovePickerOffsets : int32_t {
    KillerMoveFirst = 100003,
    KillerMoveSecond = 100002,
    CounterMove = 100001,
    BadPromotions = -10000,
};

enum PickerPhase {
    GenerateSyzygyMoves,
    Syzygy,
    GenerateMoves,
    TtMove,
    GenerateNoisyScores,
    NoisyGood,
    GenerateQuietScores,
    QuietMove,
    NoisyBad,
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
        if constexpr (moveType == movegen::MoveCapture || moveType == movegen::MoveNoisy) {
            m_skipQuiets = true;

            if (m_ttMove && !m_ttMove->isCapture())
                m_ttMove.reset();
        }
    }

    inline PickerPhase getPhase() const
    {
        return m_phase;
    }

    /* if enabled -> will skip quiets and bad promotions -> moves that are expected
     * to not be "super good" */
    inline void setSkipQuiets(bool enabled)
    {
        m_skipQuiets = enabled;
    }

    inline bool getSkipQuiets() const
    {
        return m_skipQuiets;
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
            core::getAllMoves<moveType>(board, m_moves);
            m_tail = m_moves.count();

            m_phase = PickerPhase::TtMove;

            return pickNextMove<player>(board);
        }

        case TtMove: {
            if (const auto pickedMove = pickTtMove())
                return pickedMove;

            m_phase = PickerPhase::GenerateNoisyScores;

            return pickNextMove<player>(board);
        }

        case GenerateNoisyScores: {
            generateNoisyScores(board);

            m_phase = PickerPhase::NoisyGood;

            return pickNextMove<player>(board);
        }

        case NoisyGood: {
            if (const auto pickedMove = pickNoisyMove<true>())
                return pickedMove;

            /* when running qsearch we don't want to search bad noisy - so just go straight to done */
            if constexpr (moveType == movegen::MoveNoisy || moveType == movegen::MoveCapture) {
                m_phase = PickerPhase::Done;
            } else {
                m_phase = PickerPhase::GenerateQuietScores;
            }

            return pickNextMove<player>(board);
        }

        case GenerateQuietScores: {
            if (m_skipQuiets) {
                m_phase = PickerPhase::NoisyBad;
                return pickNextMove<player>(board);
            }

            generateQuietScores<player>(board);

            m_phase = PickerPhase::QuietMove;

            return pickNextMove<player>(board);
        }

        case QuietMove: {
            if (!m_skipQuiets) {
                if (const auto pickedMove = pickQuietMove())
                    return pickedMove;
            }

            m_phase = PickerPhase::NoisyBad;

            return pickNextMove<player>(board);
        }

        case NoisyBad: {
            if (const auto pickedMove = pickNoisyMove<false>())
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
    inline movegen::Move pickMove(uint16_t pos)
    {
        std::swap(m_moves[m_head], m_moves[pos]);
        std::swap(m_scores[m_head], m_scores[pos]);

        return m_moves[m_head++];
    }

    // Syzygy moves are already sorted: return first, then second, third etc
    constexpr std::optional<movegen::Move> pickSyzygyMove()
    {
        if (m_syzygyHead == m_moves.count())
            return std::nullopt;

        auto pickedMove = m_moves[m_syzygyHead];

        m_syzygyHead++;

        return pickedMove;
    }

    constexpr std::optional<movegen::Move> pickTtMove()
    {
        if (!m_ttMove.has_value())
            return std::nullopt;

        for (uint16_t i = m_head; i < m_tail; i++) {
            if (m_moves[i] == *m_ttMove) {
                m_ttMove.reset();

                return pickMove(i);
            }
        }
        return std::nullopt;
    }

    void generateNoisyScores(const BitBoard& board)
    {
        for (uint16_t i = m_head; i < m_tail; i++) {
            if (m_moves[i].isCapture()) {
                m_scores[i] = evaluation::SeeSwap::getCaptureScore(board, m_moves[i]);
            } else if (m_moves[i].promotionType() == PromotionQueen) {
                m_scores[i] = spsa::seeQueenValue;
            } else if (m_moves[i].isPromotionMove()) {
                m_scores[i] = MovePickerOffsets::BadPromotions;
            }
        }
    }

    template<bool isGood>
    constexpr std::optional<movegen::Move> pickNoisyMove()
    {
        int32_t bestScore = std::numeric_limits<int32_t>::min();
        std::optional<uint16_t> bestMoveIndex {};

        for (uint16_t i = m_head; i < m_tail; i++) {
            if (m_moves[i].isNoisyMove()) {
                const int32_t score = m_scores[i];

                if constexpr (isGood) {
                    if (score < 0) {
                        continue;
                    }
                } else {
                    /* if we're already pruning potentially bad moves then there's no reason to
                     * pick a bad promotion - just skip it! */
                    if (m_skipQuiets && score == BadPromotions) {
                        continue;
                    }
                }

                if (score > bestScore) {
                    bestScore = score;
                    bestMoveIndex = i;
                }
            }
        }

        if (bestMoveIndex) {
            const uint16_t idx = bestMoveIndex.value();
            return pickMove(idx);
        }

        return std::nullopt;
    }

    // TODO template over bool hasCounter, to avoid redundant check?
    template<Player player>
    void generateQuietScores(const BitBoard& board)
    {
        const auto killerMoves = m_searchTables.getKillerMove(m_ply);
        const auto counterMove = m_searchTables.getCounterMove(m_prevMove.value_or(movegen::Move()));

        for (uint16_t i = m_head; i < m_tail; i++) {
            if (!m_moves[i].isQuietMove())
                continue;

            if (m_moves[i] == killerMoves.first) {
                m_scores[i] = MovePickerOffsets::KillerMoveFirst;
            } else if (m_moves[i] == killerMoves.second) {
                m_scores[i] = MovePickerOffsets::KillerMoveSecond;
            } else if (m_moves[i] == counterMove) {
                m_scores[i] = MovePickerOffsets::CounterMove;
            } else {
                const auto attacker = board.getAttackerAtSquare<player>(m_moves[i].fromSquare());
                m_scores[i] = m_searchTables.getHistoryMove(attacker.value(), m_moves[i].toPos());
            }
        }
    }

    constexpr std::optional<movegen::Move> pickQuietMove()
    {
        int32_t bestScore = std::numeric_limits<int32_t>::min();
        uint16_t bestMoveIndex = m_tail;

        for (uint16_t i = m_head; i < m_tail; i++) {
            if (m_moves[i].isQuietMove()) {
                const int32_t score = m_scores[i];

                if (score > bestScore) {
                    bestScore = score;
                    bestMoveIndex = i;
                }
            }
        }

        if (bestMoveIndex != m_tail) {
            return pickMove(bestMoveIndex);
        }

        return std::nullopt;
    }

    SearchTables& m_searchTables;

    uint8_t m_ply {};
    PickerPhase m_phase { PickerPhase::TtMove };
    std::optional<movegen::Move> m_ttMove { std::nullopt };
    std::optional<movegen::Move> m_prevMove { std::nullopt };
    bool m_skipQuiets { false };

    movegen::ValidMoves m_moves {};
    std::array<int32_t, s_maxMoves> m_scores {};

    /* Non-syzygy: pick within [m_head, m_tail), swap best to m_head, advance m_head */
    uint16_t m_head { 0 };
    uint16_t m_tail { 0 };

    /* Syzygy: pick at exactly m_syzygyHead, advance m_syzygyHead */
    uint16_t m_syzygyHead { 0 };
};
}
