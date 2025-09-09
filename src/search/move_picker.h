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

// Noisy and quiet scores are never compared
enum MovePickerOffsets : int32_t {
    GoodCapture = 50000,
    BadCapture = -50000,
    KillerMoveFirst = 100003,
    KillerMoveSecond = 100002,
    CounterMove = 100001,
    BadPromotions = -100000,
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

        m_captureScores.fill(std::numeric_limits<int16_t>::min());
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
        const auto pickedMove = m_moves[pos];

        m_moves[pos] = m_moves[m_tail - 1];
        m_scores[pos] = m_scores[m_tail - 1];
        m_captureScores[pos] = m_captureScores[m_tail - 1];

        m_tail--;

        return pickedMove;
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

        for (uint16_t i = 0; i < m_tail; i++) {
            if (m_moves[i] == *m_ttMove) {
                m_ttMove.reset();

                return pickMove(i);
            }
        }
        return std::nullopt;
    }

    void generateNoisyScores(const BitBoard& board)
    {
        for (uint16_t i = 0; i < m_tail; i++) {
            if (m_moves[i].isCapture()) {
                m_scores[i] = evaluation::SeeSwap::isGreaterThanMargin(board, m_moves[i], 0) ? 1000 : -1000;

                const auto attacker = board.getAttackerAtSquare(m_moves[i].fromSquare(), board.player).value();
                const auto victim = m_moves[i].takeEnPessant() ? (board.player == PlayerWhite ? BlackPawn : WhitePawn) : board.getTargetAtSquare(m_moves[i].toSquare(), board.player).value();
                m_captureScores[i] = m_searchTables.getCaptureHistory(attacker, m_moves[i].toPos(), victim);
            } else if (m_moves[i].promotionType() == PromotionQueen) {
                m_scores[i] = spsa::seeQueenValue;
                // Hack: give promotions a low capture score and include them in sorting by capture score
                m_captureScores[i] = std::numeric_limits<int16_t>::min() + 1000;
            } else if (m_moves[i].isPromotionMove()) {
                m_scores[i] = MovePickerOffsets::BadPromotions;
                m_captureScores[i] = std::numeric_limits<int16_t>::min() + 1000;
            }
        }
    }

    template<bool isGood>
    constexpr std::optional<movegen::Move> pickNoisyMove()
    {
        int32_t bestCaptureScore = std::numeric_limits<int32_t>::min();
        std::optional<uint16_t> bestMoveIndex {};

        for (uint16_t i = 0; i < m_tail; i++) {
            if (m_moves[i].isNoisyMove()) {
                const int32_t score = m_scores[i];
                const int16_t captureScore = m_captureScores[i];

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

                if (captureScore > bestCaptureScore) {
                    bestCaptureScore = captureScore;
                    bestMoveIndex = i;
                }
            }
        }
        return bestMoveIndex ? std::make_optional(pickMove(bestMoveIndex.value())) : std::nullopt;
    }

    // TODO template over bool hasCounter, to avoid redundant check?
    template<Player player>
    void generateQuietScores(const BitBoard& board)
    {
        const auto killerMoves = m_searchTables.getKillerMove(m_ply);

        for (uint16_t i = 0; i < m_tail; i++) {
            if (!m_moves[i].isQuietMove())
                continue;

            if (m_moves[i] == killerMoves.first) {
                m_scores[i] = MovePickerOffsets::KillerMoveFirst;
            } else if (m_moves[i] == killerMoves.second) {
                m_scores[i] = MovePickerOffsets::KillerMoveSecond;
            } else if (m_prevMove && m_moves[i] == m_searchTables.getCounterMove(m_prevMove.value())) {
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
        std::optional<uint16_t> bestMoveIndex {};

        for (uint16_t i = 0; i < m_tail; i++) {
            if (m_moves[i].isQuietMove()) {
                const int32_t score = m_scores[i];

                if (score > bestScore) {
                    bestScore = score;
                    bestMoveIndex = i;
                }
            }
        }

        return bestMoveIndex ? std::make_optional(pickMove(bestMoveIndex.value())) : std::nullopt;
    }

    SearchTables& m_searchTables;

    uint8_t m_ply {};
    PickerPhase m_phase { PickerPhase::TtMove };
    std::optional<movegen::Move> m_ttMove { std::nullopt };
    std::optional<movegen::Move> m_prevMove { std::nullopt };
    bool m_skipQuiets { false };

    movegen::ValidMoves m_moves {};
    std::array<int32_t, s_maxMoves> m_scores {};
    std::array<int16_t, s_maxMoves> m_captureScores {};
    /* Non-syzygy: pick within [0, m_tail), fill gap with last unpicked move, decrease tail */
    uint16_t m_tail {};
    /* Syzygy: pick at exactly m_head, advance m_head */
    uint16_t m_syzygyHead {};
};
}
