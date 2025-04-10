#pragma once

#include "board_defs.h"
#include <cstdint>
#include <stdint.h>

namespace evaluation {

struct Score {
    int32_t value;

    constexpr Score()
        : value(0)
    {
    }

    constexpr Score(int16_t mg, int16_t eg)
        : value((static_cast<uint16_t>(eg) << 16) | static_cast<uint16_t>(mg))
    {
    }

    [[nodiscard]] constexpr int16_t mg() const
    {
        return static_cast<int16_t>(value);
    }

    [[nodiscard]] constexpr int16_t eg() const
    {
        return static_cast<int16_t>(value >> 16);
    }

    [[nodiscard]] constexpr int32_t phaseScore(uint8_t phase) const
    {
        return ((this->mg() * phase) + (this->eg() * (s_middleGamePhase - phase))) / s_middleGamePhase;
    }

    constexpr Score& operator+=(const Score& other) noexcept
    {
        int16_t sMg = mg() + other.mg();
        int16_t sEg = eg() + other.eg();

        value = (static_cast<uint16_t>(sEg) << 16) | static_cast<uint16_t>(sMg);

        return *this;
    }

    constexpr Score& operator-=(const Score& other) noexcept
    {
        int16_t sMg = mg() - other.mg();
        int16_t sEg = eg() - other.eg();

        value = (static_cast<uint16_t>(sEg) << 16) | static_cast<uint16_t>(sMg);

        return *this;
    }

    constexpr Score operator+(const Score& other) const noexcept
    {
        int16_t sMg = mg() + other.mg();
        int16_t sEg = eg() + other.eg();

        return Score(sMg, sEg);
    }

    constexpr Score operator-(const Score& other) const noexcept
    {
        int16_t sMg = mg() - other.mg();
        int16_t sEg = eg() - other.eg();

        return Score(sMg, sEg);
    }
};
}

