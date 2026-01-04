#pragma once

#include "movegen/move_types.h"
#include <algorithm>

namespace search {

class KillerMoves {
public:
    void update(movegen::Move move, uint8_t ply)
    {
        assert(move.isQuietMove());
        m_killerMoves.at(ply) = move;
    }

    movegen::Move get(uint8_t ply) const
    {
        return m_killerMoves.at(ply);
    }

    void reset()
    {
        std::ranges::fill(m_killerMoves, movegen::Move {});
    }

private:
    std::array<movegen::Move, s_maxSearchDepth> m_killerMoves;
};

}
