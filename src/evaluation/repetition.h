#pragma once

#include "src/board_defs.h"
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

    bool isRepetition(uint64_t hash)
    {
        for (uint64_t entry : *this) {
            if (entry == hash)
                return true;
        }

        return false;
    }

    const uint64_t* begin() const
    {
        return m_repetitions.begin();
    }

    const uint64_t* end() const
    {
        return m_repetitions.begin() + m_count;
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
