#pragma once

#include "movegen/move_types.h"
#include <algorithm>
#include <utility>

namespace search {

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

    inline void clear(uint8_t ply)
    {
        m_killerMoves.at(ply) = std::pair(movegen::nullMove(), movegen::nullMove());
    }

    void reset()
    {
        std::ranges::fill(m_killerMoves, KillerMove {});
    }

private:
    /* note: +1 for clear to not cause overflow */
    std::array<KillerMove, s_maxSearchDepth + 1> m_killerMoves;
};

}

