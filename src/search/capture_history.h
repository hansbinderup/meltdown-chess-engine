
#pragma once

#include "core/bit_board.h"
#include "movegen/move_types.h"
#include "spsa/parameters.h"

#include <array>

namespace search {

const int16_t captureHistoryMaxScore = 16384;

class CaptureHistory {
public:
    inline int16_t getScore(const BitBoard& board, const movegen::Move move)
    {
        const auto attacker = board.getAttackerAtSquare(move.fromSquare(), board.player).value();
        const auto victim = move.takeEnPessant() ? (board.player == PlayerWhite ? BlackPawn : WhitePawn) : board.getTargetAtSquare(move.toSquare(), board.player).value();

        return m_captureHistoryScores[attacker][move.toPos()][victim];
    }

    template<bool isPositive>
    inline void update(const BitBoard& board, uint8_t depth, movegen::Move move)
    {
        const auto attacker = board.getAttackerAtSquare(move.fromSquare(), board.player).value();
        const auto victim = move.takeEnPessant() ? (board.player == PlayerWhite ? BlackPawn : WhitePawn) : board.getTargetAtSquare(move.toSquare(), board.player).value();

        const int16_t delta = bonus(depth); /*Always positive*/

        auto& current = m_captureHistoryScores[attacker][move.toPos()][victim];

        if constexpr (isPositive) {
            current += delta - (current * delta) / captureHistoryMaxScore;
        } else {
            current += -delta - (current * delta) / captureHistoryMaxScore;
        }
    }

private:
    static inline int16_t bonus(uint8_t depth)
    {
        return std::min<int16_t>(spsa::captureHistoryMaxBonus, spsa::captureHistoryFactor * depth * depth);
    }

    using TypeScoreArray = std::array<int16_t, 12>;
    using PosTypeScoreArray = std::array<TypeScoreArray, 64>;
    using CaptureHistoryArray = std::array<PosTypeScoreArray, 12>;

    // Indexed by [attacking piece type][target][captured type]
    CaptureHistoryArray m_captureHistoryScores {};
};
}
