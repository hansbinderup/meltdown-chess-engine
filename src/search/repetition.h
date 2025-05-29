#pragma once

#include "core/bit_board.h"
#include "core/board_defs.h"

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
        if (m_count < 2)
            return false;

        /* 3 fold repetitions allow for 2 repetitions before drawn */
        uint8_t rep = 0;

        /* m_count points at next index */
        const int16_t lastIndex = m_count - 1;

        /* only search back irresible moves */
        const int16_t end = std::max<int16_t>(0, lastIndex - board.halfMoves);

        /* check our last position (lastIndex - 1) and only check our positions (i -= 2) */
        for (int16_t i = lastIndex - 1; i >= end; i -= 2) {
            if (m_repetitions[i] == hash && ++rep >= 2)
                return true;
        }

        return false;
    }

    void reset()
    {
        std::ranges::fill(m_repetitions, 0);
        m_count = 0;
    }

private:
    uint16_t m_count {};
    std::array<uint64_t, s_maxHalfMoves> m_repetitions {};
};
