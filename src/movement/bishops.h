#pragma once

#include "src/movement/move_types.h"

#include <array>
#include <cstdint>

namespace movement {

namespace {

// Find all bishops moves based on given square
// note: this happens at compile time so couldn't care less about how efficient it is :)
constexpr uint64_t bishopsMove(uint64_t square)
{
    uint64_t moves = 0;
    int col = square % 8;
    int row = square / 8;

    const auto bishopsOffset = std::to_array<std::pair<MoveDirection, uint8_t>>({
        { MoveDirection::UpLeft(), std::min(col, 7 - row) },
        { MoveDirection::UpRight(), std::min(7 - col, 7 - row) },
        { MoveDirection::DownLeft(), std::min(col, row) },
        { MoveDirection::DownRight(), std::min(7 - col, row) },
    });

    for (const auto& [direction, distance] : bishopsOffset) {
        for (uint8_t squareOffset = 1; squareOffset <= distance; squareOffset++) {
            const uint64_t piece = 1ULL << square;
            if (direction.shiftDirection == 1) {
                moves |= piece << (squareOffset * direction.shiftAmnt);
            } else {
                moves |= piece >> (squareOffset * direction.shiftAmnt);
            }
        }
    }

    return moves;
}

// Generate a table with all knight moves
constexpr std::array<uint64_t, 64> generatebishopsTable()
{
    std::array<uint64_t, 64> table = {};
    for (uint8_t i = 0; i < 64; ++i) {
        table[i] = bishopsMove(i);
    }
    return table;
}

}

constexpr auto s_bishopsTable = generatebishopsTable();

}

