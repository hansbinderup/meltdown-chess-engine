#pragma once

#include "bit_board.h"
#include "board_defs.h"
#include <algorithm>
#include <array>
#include <cstdint>

class Repetition {
public:
    inline void add(uint64_t hash)
    {
        m_repetitions[m_count++] = hash;
    }

    inline void remove()
    {
        m_count--;
    }

    inline bool isRepetition(uint64_t hash, const BitBoard& board)
    {
        const int16_t end = std::max<int16_t>(m_count - board.halfMoves, 0);

        /* NOTE: -2: we only care about our own moves
         * NOTE: we only want to search to an irreversible position hence only check back to half moves
         * NOTE: search backwards -> more likely to be found early if it's a repeated position */
        for (int16_t i = m_count - 2; i >= end; i -= 2) {
            if (m_repetitions[i] == hash) {
                return true;
            }
        }

        return false;
    }

    inline void reset()
    {
        m_count = 0;
    }

private:
    int16_t m_count {};
    std::array<uint64_t, s_maxHalfMoves> m_repetitions {};
};
