#pragma once

#include "movegen/move_types.h"
#include <algorithm>
#include <utility>

namespace evaluation {

class KillerMoves {
public:
    using KillerMove = std::pair<movegen::Move, movegen::Move>;
    void update(movegen::Move move, uint8_t ply)
    {
        /* only quiet moves */
        if (move.isCapture()) {
            return; // nothing to do
        }

        const auto killerMove = m_killerMoves.at(ply);
        const auto updatedKillerMove = std::make_pair(move, killerMove.first);
        m_killerMoves.at(ply) = updatedKillerMove;
    }

    KillerMove get(uint8_t ply) const
    {
        return m_killerMoves.at(ply);
    }

    void reset()
    {
        std::ranges::fill(m_killerMoves, KillerMove {});
    }

private:
    std::array<KillerMove, s_maxSearchDepth> m_killerMoves;
};

}

