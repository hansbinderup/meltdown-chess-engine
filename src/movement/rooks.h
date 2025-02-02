#pragma once

#include "src/movement/move_types.h"

#include <array>
#include <cstdint>

namespace movement {

namespace {

// Find all rooks moves based on given square
// note: this happens at compile time so couldn't care less about how efficient it is :)
constexpr uint64_t rooksMove(uint64_t square)
{
    uint64_t moves = 0;
    int row = static_cast<int>(square) / 8;
    int col = static_cast<int>(square) % 8;

    const auto rooksOffset = std::to_array<std::pair<MoveDirection, uint8_t>>({
        { MoveDirection::Up(), 7 - row },
        { MoveDirection::Down(), row },
        { MoveDirection::Right(), 7 - col },
        { MoveDirection::Left(), col },
    });

    for (const auto& [direction, distance] : rooksOffset) {
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
constexpr std::array<uint64_t, 64> generateRooksTable()
{
    std::array<uint64_t, 64> table = {};
    for (uint8_t i = 0; i < 64; ++i) {
        table[i] = rooksMove(i);
    }
    return table;
}

}

constexpr auto s_rooksTable = generateRooksTable();

}

