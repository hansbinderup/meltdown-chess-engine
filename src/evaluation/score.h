#pragma once

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <optional>

using Score = int16_t;

constexpr static inline uint8_t s_maxSearchDepth { 128 };

constexpr static inline Score s_maxScore = { 30000 };
constexpr static inline Score s_minScore = { -s_maxScore };
constexpr static inline Score s_noScore = std::numeric_limits<Score>::min();

constexpr static inline Score s_mateValue { 20000 };
constexpr static inline Score s_mateScore { s_mateValue - s_maxSearchDepth };

/* returns a score adjusted relative to ply (mate sooner = better) */
constexpr inline int16_t scoreRelative(int16_t score, uint8_t ply)
{
    if (score == s_noScore) {
        return s_noScore;
    } else if (score > s_mateScore) {
        score -= ply;
    } else if (score < -s_mateScore) {
        score += ply;
    }

    return score;
}

/* Returns a score adjusted absolute, removing any ply shift */
constexpr inline int16_t scoreAbsolute(int16_t score, uint8_t ply)
{
    if (score == s_noScore) {
        return s_noScore;
    } else if (score > s_mateScore) {
        score += ply;
    } else if (score < -s_mateScore) {
        score -= ply;
    }

    return score;
}

constexpr inline std::optional<int8_t> scoreMateDistance(int16_t score)
{
    const int16_t absScore = std::abs(score);
    if (absScore >= s_mateScore) {
        return std::copysign((s_mateValue - absScore / 2) + 1, score);
    }

    return std::nullopt;
}

constexpr inline bool scoreIsMate(Score score)
{
    return std::abs(score) >= s_mateScore;
}
