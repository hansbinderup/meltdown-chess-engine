#pragma once

#include "board_defs.h"

#include "bit_board.h"
#include "helpers/bit_operations.h"
#include "movegen/bishops.h"
#include "movegen/kings.h"
#include "movegen/knights.h"
#include "movegen/rooks.h"

#include <cstdint>

namespace attackgen {

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
        attacks |= movegen::getKnightMoves(from);
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
        attacks |= movegen::getRookMoves(from, occupancy);
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
        attacks |= movegen::getBishopMoves(from, occupancy);
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
        attacks |= movegen::getRookMoves(from, occupancy);
        attacks |= movegen::getBishopMoves(from, occupancy);
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

    return movegen::getKingMoves(helper::lsbToPosition(king));
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
        attacks |= getWhitePawnAttacks(board);
    } else {
        attacks |= getBlackPawnAttacks(board);
    }

    attacks |= getKnightAttacks<player>(board);
    attacks |= getRookAttacks<player>(board);
    attacks |= getBishopAttacks<player>(board);
    attacks |= getQueenAttacks<player>(board);
    attacks |= getKingAttacks<player>(board);

    return attacks;
}

}
