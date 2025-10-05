
#pragma once

#include "core/bit_board.h"
#include "movegen/move_types.h"
#include "spsa/parameters.h"

#include <array>

namespace search {

class CaptureHistory {
public:
    inline std::optional<int16_t> getScore(const BitBoard& board, const movegen::Move move)
    {
        if (!move.isNoisyMove()) {
            return std::nullopt;
        }

        const auto attacker = board.getAttackerAtSquare(move.fromSquare(), board.player).value();
        const auto victim = board.getVictim(move);

        return m_captureHistoryScores[attacker][move.toPos()][victim];
    }

    template<bool isPositive>
    inline void update(const BitBoard& board, uint8_t depth, movegen::Move move)
    {
        assert(move.isNoisyMove());

        const auto attacker = board.getAttackerAtSquare(move.fromSquare(), board.player).value();
        const auto victim = board.getVictim(move);

        const int16_t delta = bonus(depth); /*Always positive*/

        auto& current = m_captureHistoryScores[attacker][move.toPos()][victim];

        int32_t newVal = current + (isPositive ? delta : -delta);
        newVal -= (current * delta) / spsa::captureHistoryMaxScore;
        current = std::clamp<int32_t>(newVal, -spsa::captureHistoryMaxScore, spsa::captureHistoryMaxScore);
    }

private:
    static inline int16_t bonus(uint8_t depth)
    {
        return std::min<int16_t>(spsa::captureHistoryMaxBonus, depth * depth);
    }

    using TypeScoreArray = std::array<int16_t, 12>;
    using PosTypeScoreArray = std::array<TypeScoreArray, 64>;
    using CaptureHistoryArray = std::array<PosTypeScoreArray, 12>;

    // Indexed by [attacking piece type][target][captured type]
    CaptureHistoryArray m_captureHistoryScores {};
};
}
