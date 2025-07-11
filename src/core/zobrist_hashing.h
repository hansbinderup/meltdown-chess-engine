#pragma once

#include "core/bit_board.h"
#include "core/board_defs.h"
#include "magic_enum/magic_enum.hpp"
#include "utils/bit_operations.h"
#include <array>

namespace core {

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

constexpr static inline uint64_t splitMixHash(uint64_t value)
{
    return splitmix64(value);
}

constexpr static inline void hashPiece(Piece piece, BoardPosition pos, uint64_t& hash)
{
    hash ^= s_pieceHashTable[piece][pos];
}

constexpr static inline void hashEnpessant(BoardPosition pos, uint64_t& hash)
{
    hash ^= s_enpessantHashTable[pos];
}

constexpr static inline void hashCastling(uint64_t castleFlags, uint64_t& hash)
{
    hash ^= s_castlingHashTable[castleFlags];
}

constexpr static inline void hashPlayer(uint64_t& hash)
{
    hash ^= s_playerKey;
}

constexpr uint64_t generateHash(const BitBoard& board)
{
    uint64_t hash = 0;
    for (const auto pieceEnum : magic_enum::enum_values<Piece>()) {
        uint64_t piece = board.pieces[pieceEnum];

        utils::bitIterate(piece, [&](BoardPosition pos) {
            hash ^= s_pieceHashTable[pieceEnum][pos];
        });
    }

    hash ^= s_castlingHashTable[board.castlingRights];

    if (board.enPessant.has_value()) {
        hash ^= s_enpessantHashTable[board.enPessant.value()];
    }

    /* only need to hash black as we only have two players */
    if (board.player == PlayerBlack) {
        hash ^= s_playerKey;
    }

    return hash;
}

static inline uint64_t generateKingPawnHash(const BitBoard& board)
{
    uint64_t hash = 0;
    constexpr auto kpPieces = std::to_array<Piece>({ WhitePawn, WhiteKing, BlackPawn, BlackKing });
    for (const auto pieceEnum : kpPieces) {
        uint64_t piece = board.pieces[pieceEnum];

        utils::bitIterate(piece, [&](BoardPosition pos) {
            hash ^= s_pieceHashTable[pieceEnum][pos];
        });
    }

    return hash;
}

/* slightly more efficient way to generate material hash */
static inline uint64_t generateMaterialHash(const BitBoard& board)
{
    /* we don't care about kings here - they're always present */
    constexpr auto materialPieces = std::to_array<Piece>({ WhitePawn, WhiteKnight, WhiteBishop, WhiteRook, WhiteQueen,
        BlackPawn, BlackKnight, BlackBishop, BlackRook, BlackQueen });

    uint64_t hash = 0;
    for (const auto pieceEnum : materialPieces) {
        uint8_t count = std::popcount(board.pieces[pieceEnum]);
        hash ^= s_pieceHashTable[pieceEnum][count];
    }

    return hash;
}

}
