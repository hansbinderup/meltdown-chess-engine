#pragma once

#include "board_defs.h"

#include "bit_board.h"
#include "helpers/bit_operations.h"
#include "movement/bishops.h"
#include "movement/kings.h"
#include "movement/knights.h"
#include "movement/rooks.h"

#include <bit>
#include <cstdint>

namespace gen {

namespace {

// most valuable victim & less valuable attacker

/*
    (Victims) Pawn Knight Bishop   Rook  Queen   King
  (Attackers)
        Pawn   105    205    305    405    505    605
      Knight   104    204    304    404    504    604
      Bishop   103    203    303    403    503    603
        Rook   102    202    302    402    502    602
       Queen   101    201    301    401    501    601
        King   100    200    300    400    500    600
*/

using MvvTableEntry = std::array<int32_t, 12>;
constexpr auto s_mvvLvaTable = std::to_array<MvvTableEntry>(
    {
        { 105, 205, 305, 405, 505, 605, 105, 205, 305, 405, 505, 605 },
        { 104, 204, 304, 404, 504, 604, 104, 204, 304, 404, 504, 604 },
        { 103, 203, 303, 403, 503, 603, 103, 203, 303, 403, 503, 603 },
        { 102, 202, 302, 402, 502, 602, 102, 202, 302, 402, 502, 602 },
        { 101, 201, 301, 401, 501, 601, 101, 201, 301, 401, 501, 601 },
        { 100, 200, 300, 400, 500, 600, 100, 200, 300, 400, 500, 600 },
        { 105, 205, 305, 405, 505, 605, 105, 205, 305, 405, 505, 605 },
        { 104, 204, 304, 404, 504, 604, 104, 204, 304, 404, 504, 604 },
        { 103, 203, 303, 403, 503, 603, 103, 203, 303, 403, 503, 603 },
        { 102, 202, 302, 402, 502, 602, 102, 202, 302, 402, 502, 602 },
        { 101, 201, 301, 401, 501, 601, 101, 201, 301, 401, 501, 601 },
        { 100, 200, 300, 400, 500, 600, 100, 200, 300, 400, 500, 600 },
    });
}

constexpr static inline int32_t getMvvLvaScore(Piece attacker, Piece victim)
{
    return s_mvvLvaTable.at(attacker).at(victim);
}

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
        attacks |= movement::s_knightsTable.at(from);
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
        attacks |= movement::getRookAttacks(from, occupancy);
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
        attacks |= movement::getBishopAttacks(from, occupancy);
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
        attacks |= movement::getRookAttacks(from, occupancy);
        attacks |= movement::getBishopAttacks(from, occupancy);
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
    return movement::getKingAttacks(from);
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
