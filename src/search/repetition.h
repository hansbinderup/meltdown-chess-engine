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

    bool isRepetition(const BitBoard& board, uint64_t hash, uint8_t ply)
    {
        uint8_t reps = 0;

        /* count back the history checking only our positions (hence -2) until we reach a
         * non-reversible position (half-moves) */
        for (int16_t i = m_count - 2; i >= 0; i -= 2) {

            /* No draw can occur before a zeroing move */
            if (i < (m_count - board.halfMoves))
                break;

            /* if the same position occurred after the root (i > m_count - ply),
             * a twofold repetition is enough to consider it a potential draw  during search
             * otherwise, count full repetitions and return true on the third (legal threefold draw) */
            if (m_repetitions[i] == hash && (i > m_count - ply || ++reps == 2))
                return true;
        }

        return false;
    }

    void reset()
    {
        m_count = 0;
    }

private:
    int16_t m_count {};
    std::array<uint64_t, s_maxHalfMoves> m_repetitions {};
};
