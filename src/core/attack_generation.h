#pragma once

#include "core/board_defs.h"

#include "core/bit_board.h"
#include "movegen/bishops.h"
#include "movegen/kings.h"
#include "movegen/knights.h"
#include "movegen/rooks.h"
#include "utils/bit_operations.h"

#include <cstdint>

namespace attackgen {

template<Player player>
constexpr static inline uint64_t getKnightAttacks(const BitBoard& board)
{
    constexpr Piece ourKnights = player == PlayerWhite ? WhiteKnight : BlackKnight;
    const uint64_t knights = board.pieces[ourKnights];

    uint64_t attacks = {};
    utils::bitIterate(knights, [&](BoardPosition from) {
        attacks |= movegen::getKnightMoves(from);
    });

    return attacks;
}

template<Player player>
constexpr static inline uint64_t getRookAttacks(const BitBoard& board)
{
    constexpr Piece ourRooks = player == PlayerWhite ? WhiteRook : BlackRook;
    const uint64_t rooks = board.pieces[ourRooks];
    const uint64_t occupancy = board.occupation[Both];

    uint64_t attacks = {};
    utils::bitIterate(rooks, [&](BoardPosition from) {
        attacks |= movegen::getRookMoves(from, occupancy);
    });

    return attacks;
}

template<Player player>
constexpr static inline uint64_t getBishopAttacks(const BitBoard& board)
{
    constexpr Piece ourBishops = player == PlayerWhite ? WhiteBishop : BlackBishop;
    const uint64_t bishops = board.pieces[ourBishops];
    const uint64_t occupancy = board.occupation[Both];

    uint64_t attacks = {};
    utils::bitIterate(bishops, [&](BoardPosition from) {
        attacks |= movegen::getBishopMoves(from, occupancy);
    });

    return attacks;
}

template<Player player>
constexpr static inline uint64_t getQueenAttacks(const BitBoard& board)
{
    constexpr Piece ourQueens = player == PlayerWhite ? WhiteQueen : BlackQueen;
    const uint64_t queens = board.pieces[ourQueens];
    const uint64_t occupancy = board.occupation[Both];

    uint64_t attacks = {};
    utils::bitIterate(queens, [&](BoardPosition from) {
        attacks |= movegen::getRookMoves(from, occupancy);
        attacks |= movegen::getBishopMoves(from, occupancy);
    });

    return attacks;
}

template<Player player>
constexpr static inline uint64_t getKingAttacks(const BitBoard& board)
{
    constexpr Piece ourKing = player == PlayerWhite ? WhiteKing : BlackKing;
    const uint64_t king = board.pieces[ourKing];

    if (king == 0) {
        return 0;
    }

    return movegen::getKingMoves(utils::lsbToPosition(king));
}

template<Player player>
constexpr static inline uint64_t getPawnAttacks(const BitBoard& board)
{
    if constexpr (player == PlayerWhite) {
        const uint64_t pawns = board.pieces[WhitePawn];
        uint64_t attacks = ((pawns & ~s_aFileMask) << 7);
        attacks |= ((pawns & ~s_hFileMask) << 9);

        return attacks;
    } else {
        const uint64_t pawns = board.pieces[BlackPawn];
        uint64_t attacks = ((pawns & ~s_aFileMask) >> 9);
        attacks |= ((pawns & ~s_hFileMask) >> 7);

        return attacks;
    }
}

template<Player player>
constexpr uint64_t getAllAttacks(const BitBoard& board)
{
    uint64_t attacks = getPawnAttacks<player>(board);
    attacks |= getKnightAttacks<player>(board);
    attacks |= getRookAttacks<player>(board);
    attacks |= getBishopAttacks<player>(board);
    attacks |= getQueenAttacks<player>(board);
    attacks |= getKingAttacks<player>(board);

    return attacks;
}

}
