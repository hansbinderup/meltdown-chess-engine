#pragma once

#include "board_defs.h"
#include "magic_enum/magic_enum.hpp"

#include <algorithm>
#include <cstdint>

namespace evaluation::gamephase {

namespace {

constexpr uint8_t s_mgLimit { 24 };

struct Weight {
    uint8_t mg;
    uint8_t eg;
};

[[nodiscard]] constexpr Weight computeWeight(uint8_t materialPhase)
{
    /* we have to limit the score in case of early promotions */
    const uint8_t mgPhase = std::min(materialPhase, s_mgLimit);
    const uint8_t egPhase = s_mgLimit - mgPhase;

    return { mgPhase, egPhase };
}

}

constexpr std::array<uint8_t, magic_enum::enum_count<Piece>()> s_materialPhaseScore = {
    0, 1, 1, 2, 4, 0, /* white pieces */
    0, 1, 1, 2, 4, 0 /* black pieces */
};

enum Phases : uint8_t {
    GamePhaseMg = 0,
    GamePhaseEg,
};

struct Score {
    int32_t mg {};
    int32_t eg {};
    uint8_t materialScore {};

    constexpr Score operator+(const Score& other) const noexcept
    {
        return { static_cast<int32_t>(mg + other.mg), static_cast<int32_t>(eg + other.eg), static_cast<uint8_t>(materialScore + other.materialScore) };
    }

    constexpr Score& operator+=(const Score& other) noexcept
    {
        mg += other.mg;
        eg += other.eg;
        materialScore += other.materialScore;
        return *this;
    }

    /* NOTE: material score should still be incremented here! */
    constexpr Score operator-(const Score& other) const noexcept
    {
        return { static_cast<int32_t>(mg - other.mg), static_cast<int32_t>(eg - other.eg), static_cast<uint8_t>(materialScore + other.materialScore) };
    }

    /* NOTE: material score should still be incremented here! */
    constexpr Score& operator-=(const Score& other) noexcept
    {
        mg -= other.mg;
        eg -= other.eg;
        materialScore += other.materialScore;
        return *this;
    }
};

[[nodiscard]] constexpr int32_t taperedScore(const Score& score)
{
    const auto weight = computeWeight(score.materialScore);
    return (score.mg * weight.mg + score.eg * weight.eg) / s_mgLimit;
}

}

