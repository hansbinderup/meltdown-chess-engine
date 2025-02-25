#pragma once

#include "magic_enum/magic_enum.hpp"
#include "src/bit_board.h"
#include "src/board_defs.h"
#include <array>

namespace engine {

namespace {

/* pseudo random number generated at compile time */
constexpr uint64_t splitmix64(uint64_t& seed)
{
    seed += 0x9E3779B97F4A7C15; // Large constant
    uint64_t result = seed;
    result = (result ^ (result >> 30)) * 0xBF58476D1CE4E5B9;
    result = (result ^ (result >> 27)) * 0x94D049BB133111EB;
    return result ^ (result >> 31);
}

constexpr auto createPieceHashTables()
{
    using PieceHash = std::array<uint64_t, s_amountSquares>;
    std::array<PieceHash, magic_enum::enum_count<Piece>()> table;

    uint64_t seed = 0xDEADBEEFCAFEBABE;
    for (auto& pieceTable : table) {
        for (auto& hash : pieceTable) {
            hash = splitmix64(seed);
        }
    }

    return table;
}

constexpr auto createEnpessantHashTable()
{
    std::array<uint64_t, s_amountSquares> table;

    uint64_t seed = 0xC0FFEE123456789A;
    for (auto& hash : table) {
        hash = splitmix64(seed);
    }

    return table;
}

constexpr auto createCastlingHashTable()
{
    std::array<uint64_t, 16> table;

    uint64_t seed = 0x5D391D7E1A2B3C4D;
    for (auto& hash : table) {
        hash = splitmix64(seed);
    }

    return table;
}

constexpr auto createPlayerKey()
{

    uint64_t seed = 0x7F4A9E3779B97C15;
    return splitmix64(seed);
}

constexpr auto s_pieceHashTable = createPieceHashTables();
constexpr auto s_enpessantHashTable = createEnpessantHashTable();
constexpr auto s_castlingHashTable = createCastlingHashTable();
constexpr auto s_playerKey = createPlayerKey();

}

constexpr uint64_t generateHashKey(const BitBoard& board)
{
    uint64_t key = 0;
    for (const auto pieceEnum : magic_enum::enum_values<Piece>()) {
        uint64_t piece = board.pieces[pieceEnum];

        while (piece) {
            const int square = std::countr_zero(piece);
            piece &= piece - 1;
            key ^= s_pieceHashTable[pieceEnum][square];
        }
    }

    /* TODO: add castling hash! Not currently in a format where that makes sense to do */

    if (board.enPessant.has_value()) {
        const int square = std::countr_zero(board.enPessant.value());
        key ^= s_enpessantHashTable[square];
    }

    /* only need to hash black as we only have two players */
    if (board.player == Player::Black) {
        key ^= s_playerKey;
    }

    return key;
}

}
