#pragma once

#include "src/bit_board.h"
#include "src/movement/move_types.h"

namespace parsing {

namespace {

/*
 * Compare moves by only looking at positioning
 * We need to apply the same masks as the bitboard uses to emulate their moves
 */
constexpr static inline bool compareMove(const movement::Move& move, uint8_t from, uint8_t to)
{
    return (move.from == from) && (move.to == to);
}

}

static inline std::optional<movement::Move> moveFromString(const BitBoard& board, std::string_view sv)
{
    if (sv.size() < 4) {
        return std::nullopt;
    }

    // quick and dirty way to transform move string to integer values
    const uint8_t fromIndex = (sv.at(0) - 'a') + (sv.at(1) - '1') * 8;
    const uint8_t toIndex = (sv.at(2) - 'a') + (sv.at(3) - '1') * 8;

    const auto allMoves = board.getAllMoves().getMoves();
    for (const auto& move : allMoves) {
        if (compareMove(move, fromIndex, toIndex)) {
            return move;
        }
    }

    // move is not a valid move.. return a move with no flags is better than nothing - good for testing and debugging
    return movement::Move { fromIndex, toIndex };
}

}

