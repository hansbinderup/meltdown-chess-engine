#pragma once

#include "bit_board.h"
#include "board_defs.h"
#include <algorithm>
#include <array>
#include <cstdint>

class Repetition {
public:
    void add(uint64_t hash)
    {
        m_repetitions[m_count++] = hash;
    }

    void remove()
    {
        m_count--;
    }

    bool isRepetition(uint64_t hash, const BitBoard& board)
    {
        uint8_t reps = 0;
        const int16_t end = std::max<int16_t>(m_count - board.halfMoves, 0);

        /* NOTE: -1: we only care about our own moves
         * NOTE: we only want to search to an irreversible position hence only check back to half moves
         * NOTE: search backwards -> more likely to be found early if it's a repeated position */
        for (int16_t i = m_count - 1; i >= end; i -= 2) {
            if (m_repetitions[i] == hash) {
                if (++reps == 2)
                    return true;
            }
        }

        return false;
    }

    void reset()
    {
        m_count = 0;
    }

private:
    uint16_t m_count {};
    std::array<uint64_t, s_maxHalfMoves> m_repetitions {};
};
