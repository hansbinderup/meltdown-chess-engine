#pragma once

#include <cstdint>

using Score = int16_t;

constexpr static inline Score s_maxScore = { 30000 };
constexpr static inline Score s_minScore = { -s_maxScore };

constexpr static inline Score s_mateValue { 20000 };
constexpr static inline Score s_mateScore { s_mateValue - 1000 };

/* returns a score adjusted relative to ply (mate sooner = better) */
constexpr inline int16_t scoreRelative(int16_t score, uint8_t ply)
{
    if (score > s_mateScore) {
        score -= ply;
    } else if (score < -s_mateScore) {
        score += ply;
    }

    return score;
}

/* Returns a score adjusted absolute, removing any ply shift */
constexpr inline int16_t scoreAbsolute(int16_t score, uint8_t ply)
{

    if (score > s_mateScore) {
        score += ply;
    } else if (score < -s_mateScore) {
        score -= ply;
    }

    return score;
}

