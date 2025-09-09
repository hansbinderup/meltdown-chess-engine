
#pragma once

#include "core/bit_board.h"
#include "movegen/move_types.h"
#include "spsa/parameters.h"

#include <array>

namespace search {

const int16_t captureHistoryMaxScore = 16384;

class CaptureHistory {
public:
    inline int16_t getScore(Piece attacker, BoardPosition toPos, Piece victim)
    {
        return m_captureHistoryScores[attacker][toPos][victim];
    }

    template<bool isPositive>
    inline void update(const BitBoard& board, uint8_t depth, movegen::Move move)
    {
        if constexpr (isPositive) {
            applyBonus(board, captureHistoryScore(depth), move);
        } else {
            applyBonus(board, -captureHistoryScore(depth), move);
        }
    }

private:
    inline void applyBonus(const BitBoard& board, int16_t bonus, movegen::Move move)
    {
        const auto attacker = board.getAttackerAtSquare(move.fromSquare(), board.player).value();
        const auto victim = move.takeEnPessant() ? (board.player == PlayerWhite ? BlackPawn : WhitePawn) : board.getTargetAtSquare(move.toSquare(), board.player).value();

        int16_t old = m_captureHistoryScores[attacker][move.toPos()][victim];

        m_captureHistoryScores[attacker][move.toPos()][victim] = old + bonus - (old * std::abs(bonus)) / captureHistoryMaxScore;
    }

    static inline int16_t captureHistoryScore(uint8_t depth)
    {
        return std::min<int16_t>(spsa::captureHistoryMaxBonus, spsa::captureHistoryFactor * depth * depth);
    }

    using TypeScoreArray = std::array<int16_t, 12>;
    using PosTypeScoreArray = std::array<TypeScoreArray, 64>;
    using CaptureHistoryArray = std::array<PosTypeScoreArray, 12>;

    // Indexed by [capturing piece type][target][captured type]
    CaptureHistoryArray m_captureHistoryScores {};
};
}
