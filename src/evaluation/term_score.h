#pragma once

#include "core/board_defs.h"
#include <cstdint>
#include <stdint.h>

namespace evaluation {

/*
 * these values seem pretty standard for a few engines out there
 * borrow them for now and consider if they should be tunable (probably not)
 *
 * NOTE: SCB -> Single Color Bishop
 */
enum ScaleFactor : uint8_t {
    Draw = 0,
    PawnBonus = 8,
    ScbBishopsOnly = 64,
    ScbOneKnight = 106,
    ScbOneRook = 96,
    LoneQueen = 88,
    BaseScale = 96,
    Normal = 128,
    LargePawnAdv = 144,
};

struct TermScore {
    uint32_t value;

    constexpr TermScore()
        : value(0)
    {
    }

    constexpr TermScore(int16_t mg, int16_t eg)
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

    [[nodiscard]] constexpr Score phaseScore(uint8_t phase, uint8_t egFactor) const
    {
        return ((this->mg() * phase)
                   + (this->eg() * (s_middleGamePhase - phase) * egFactor / ScaleFactor::Normal))
            / s_middleGamePhase;
    }

    [[nodiscard]] constexpr Score phaseScore(uint8_t phase) const
    {
        return phaseScore(phase, ScaleFactor::Normal);
    }

    constexpr TermScore& operator+=(const TermScore& other) noexcept
    {
        int16_t sMg = mg() + other.mg();
        int16_t sEg = eg() + other.eg();

        value = (static_cast<uint16_t>(sEg) << 16) | static_cast<uint16_t>(sMg);

        return *this;
    }

    constexpr TermScore& operator-=(const TermScore& other) noexcept
    {
        int16_t sMg = mg() - other.mg();
        int16_t sEg = eg() - other.eg();

        value = (static_cast<uint16_t>(sEg) << 16) | static_cast<uint16_t>(sMg);

        return *this;
    }

    constexpr TermScore& operator*=(int multiply) noexcept
    {
        int16_t sMg = mg() * multiply;
        int16_t sEg = eg() * multiply;

        value = (static_cast<uint16_t>(sEg) << 16) | static_cast<uint16_t>(sMg);

        return *this;
    }

    constexpr TermScore operator+(const TermScore& other) const noexcept
    {
        int16_t sMg = mg() + other.mg();
        int16_t sEg = eg() + other.eg();

        return TermScore(sMg, sEg);
    }

    constexpr TermScore operator-(const TermScore& other) const noexcept
    {
        int16_t sMg = mg() - other.mg();
        int16_t sEg = eg() - other.eg();

        return TermScore(sMg, sEg);
    }

    constexpr TermScore operator*(int multiply) const noexcept
    {
        int16_t sMg = mg() * multiply;
        int16_t sEg = eg() * multiply;

        return TermScore(sMg, sEg);
    }
};
}
