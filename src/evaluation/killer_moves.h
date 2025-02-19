#pragma once

#include "src/movement/move_types.h"
#include <algorithm>
#include <utility>

namespace heuristic {

class KillerMoves {
public:
    using KillerMove = std::pair<movement::Move, movement::Move>;
    void update(const movement::Move& move, uint8_t ply)
    {
        if (!move.isCapture()) {
            return; // nothing to do
        }

        const auto killerMove = m_killerMoves.at(ply);
        const auto updatedKillerMove = std::make_pair(move, killerMove.first);
        m_killerMoves.at(ply) = updatedKillerMove;
    }

    KillerMove get(uint8_t ply)
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

