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
        if (board.halfMoves <= 2)
            return false;

        auto itr = m_repetitions.begin() + m_count - 1;
        const auto end = itr - board.halfMoves;

        while (itr >= end) {
            if (*itr == hash) {
                return true;
            }

            itr -= 2;
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
