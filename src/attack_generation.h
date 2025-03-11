#pragma once

#include "board_defs.h"

#include "bit_board.h"
#include "helpers/bit_operations.h"
#include "movegen/bishops.h"
#include "movegen/kings.h"
#include "movegen/knights.h"
#include "movegen/rooks.h"

#include <bit>
#include <cstdint>

namespace gen {

template<Player player>
constexpr static inline uint64_t getKnightAttacks(const BitBoard& board)
{
    uint64_t knights {};
    uint64_t attacks = {};

    if constexpr (player == PlayerWhite) {
        knights = board.pieces[WhiteKnight];
    } else {
        knights = board.pieces[BlackKnight];
    }

    helper::bitIterate(knights, [&](BoardPosition from) {
        attacks |= movegen::getKnightAttacks(from);
    });

    return attacks;
}

template<Player player>
constexpr static inline uint64_t getRookAttacks(const BitBoard& board)
{
    uint64_t rooks {};
    uint64_t attacks {};
    const uint64_t occupancy = board.occupation[Both];

    if constexpr (player == PlayerWhite) {
        rooks = board.pieces[WhiteRook];
    } else {
        rooks = board.pieces[BlackRook];
    }

    helper::bitIterate(rooks, [&](BoardPosition from) {
        attacks |= movegen::getRookAttacks(from, occupancy);
    });

    return attacks;
}

template<Player player>
constexpr static inline uint64_t getBishopAttacks(const BitBoard& board)
{
    uint64_t bishops {};
    uint64_t attacks = {};
    const uint64_t occupancy = board.occupation[Both];

    if constexpr (player == PlayerWhite) {
        bishops = board.pieces[WhiteBishop];
    } else {
        bishops = board.pieces[BlackBishop];
    }

    helper::bitIterate(bishops, [&](BoardPosition from) {
        attacks |= movegen::getBishopAttacks(from, occupancy);
    });

    return attacks;
}

template<Player player>
constexpr static inline uint64_t getQueenAttacks(const BitBoard& board)
{
    uint64_t queens {};
    uint64_t attacks = {};
    const uint64_t occupancy = board.occupation[Both];

    if constexpr (player == PlayerWhite) {
        queens = board.pieces[WhiteQueen];
    } else {
        queens = board.pieces[BlackQueen];
    }

    helper::bitIterate(queens, [&](BoardPosition from) {
        attacks |= movegen::getRookAttacks(from, occupancy);
        attacks |= movegen::getBishopAttacks(from, occupancy);
    });

    return attacks;
}

template<Player player>
constexpr static inline uint64_t getKingAttacks(const BitBoard& board)
{
    uint64_t king {};

    if constexpr (player == PlayerWhite) {
        king = board.pieces[WhiteKing];
    } else {
        king = board.pieces[BlackKing];
    }

    if (king == 0) {
        return 0;
    }

    const int from = std::countr_zero(king);
    return movegen::getKingAttacks(from);
}

constexpr static inline uint64_t getWhitePawnAttacks(const BitBoard& board)
{
    const uint64_t pawns = board.pieces[WhitePawn];
    uint64_t attacks = ((pawns & ~s_aFileMask) << 7);
    attacks |= ((pawns & ~s_hFileMask) << 9);

    return attacks;
}

constexpr static inline uint64_t getBlackPawnAttacks(const BitBoard& board)
{
    const uint64_t pawns = board.pieces[BlackPawn];
    uint64_t attacks = ((pawns & ~s_aFileMask) >> 9);
    attacks |= ((pawns & ~s_hFileMask) >> 7);

    return attacks;
}

template<Player player>
constexpr uint64_t getAllAttacks(const BitBoard& board)
{
    uint64_t attacks {};

    if constexpr (player == PlayerWhite) {
        attacks |= gen::getWhitePawnAttacks(board);
    } else {
        attacks |= gen::getBlackPawnAttacks(board);
    }

    attacks |= gen::getKnightAttacks<player>(board);
    attacks |= gen::getRookAttacks<player>(board);
    attacks |= gen::getBishopAttacks<player>(board);
    attacks |= gen::getQueenAttacks<player>(board);
    attacks |= gen::getKingAttacks<player>(board);

    return attacks;
}

}
