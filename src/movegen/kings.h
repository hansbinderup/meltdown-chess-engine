
#pragma once

#include "board_defs.h"
#include <array>
#include <cstdint>

namespace movegen {

namespace {

// Find all king moves based on given square
// note: this happens at compile time so couldn't care less about how efficient it is :)
constexpr uint64_t kingsMove(int square)
{
    uint64_t moves = 0;
    int row = square / 8;
    int col = square % 8;

    constexpr int kingsOffset[8][2] = {
        { 0, 1 }, { 0, -1 }, { -1, -1 }, { -1, 0 },
        { -1, 1 }, { 1, -1 }, { 1, 0 }, { 1, 1 }
    };

    for (auto [dr, dc] : kingsOffset) {
        int newRow = row + dr;
        int newCol = col + dc;
        if (newRow >= 0 && newRow < 8 && newCol >= 0 && newCol < 8) {
            moves |= 1ULL << (newRow * 8 + newCol);
        }
    }

    return moves;
}

// Generate a table with all kings moves
constexpr std::array<uint64_t, 64> generateKingsTable()
{
    std::array<uint64_t, 64> table = {};
    for (uint8_t i = 0; i < 64; ++i) {
        table[i] = kingsMove(i);
    }
    return table;
}

constexpr auto s_kingsTable = generateKingsTable();

}

static inline uint64_t getKingMoves(BoardPosition pos)
{
    return s_kingsTable[pos];
}

}

