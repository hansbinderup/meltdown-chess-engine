#pragma once

#include <array>
#include <cstdint>

namespace movement {

namespace {

// Find all knights moves based on given square
// note: this happens at compile time so couldn't care less about how efficient it is :)
constexpr uint64_t knightsMove(int square)
{
    uint64_t moves = 0;
    int row = square / 8;
    int col = square % 8;

    constexpr int knightsOffset[8][2] = {
        { -2, -1 }, { -2, 1 }, { 2, -1 }, { 2, 1 },
        { -1, -2 }, { -1, 2 }, { 1, -2 }, { 1, 2 }
    };

    for (auto [dr, dc] : knightsOffset) {
        int newRow = row + dr;
        int newCol = col + dc;
        if (newRow >= 0 && newRow < 8 && newCol >= 0 && newCol < 8) {
            moves |= 1ULL << (newRow * 8 + newCol);
        }
    }

    return moves;
}

// Generate a table with all knight moves
constexpr std::array<uint64_t, 64> generateKnightsTable()
{
    std::array<uint64_t, 64> table = {};
    for (uint8_t i = 0; i < 64; ++i) {
        table[i] = knightsMove(i);
    }
    return table;
}

}

constexpr auto s_knightsTable = generateKnightsTable();

}

