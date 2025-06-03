#pragma once

#include "movegen/move_types.h"
#include <algorithm>

namespace search {

class CounterMoves {
public:
    inline void update(movegen::Move prevMove, movegen::Move counterMove)
    {
        /* only quiet moves */
        if (counterMove.isCapture()) {
            return; // nothing to do
        }
        m_counterMoves.at(prevMove.fromPos()).at(prevMove.toPos()) = counterMove;
    }

    inline movegen::Move get(movegen::Move prevMove) const
    {
        return m_counterMoves.at(prevMove.fromPos()).at(prevMove.toPos());
    }

    inline void reset()
    {
        std::ranges::fill(m_counterMoves, CounterEntry {});
    }

private:
    using CounterEntry = std::array<movegen::Move, s_amountSquares>;
    std::array<CounterEntry, s_amountSquares> m_counterMoves;
};

}
