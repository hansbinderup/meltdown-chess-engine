#pragma once

#include "board_defs.h"

#include "src/bit_board.h"
#include "src/movement/bishops.h"
#include "src/movement/kings.h"
#include "src/movement/knights.h"
#include "src/movement/rooks.h"

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

using MvvTableEntry = std::array<int16_t, 12>;
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

constexpr static inline int16_t getMvvLvaScore(Piece attacker, Piece victim)
{
    return s_mvvLvaTable.at(attacker).at(victim);
}

constexpr static inline uint64_t getKnightAttacks(const BitBoard& board, std::optional<Player> player = std::nullopt)
{
    uint64_t knights = player.value_or(board.player) == Player::White ? board.pieces[WhiteKnight] : board.pieces[BlackKnight];
    uint64_t attacks = {};

    while (knights) {
        const int from = std::countr_zero(knights);
        knights &= knights - 1;

        attacks |= movement::s_knightsTable.at(from);
    }

    return attacks;
}

constexpr static inline uint64_t getRookAttacks(const BitBoard& board, std::optional<Player> player = std::nullopt)
{
    uint64_t rooks = player.value_or(board.player) == Player::White ? board.pieces[WhiteRook] : board.pieces[BlackRook];
    const uint64_t occupancy = board.occupation[Both];

    uint64_t attacks = {};
    while (rooks) {
        const int from = std::countr_zero(rooks);
        rooks &= rooks - 1;

        attacks |= movement::getRookAttacks(from, occupancy);
    }

    return attacks;
}

constexpr static inline uint64_t getBishopAttacks(const BitBoard& board, std::optional<Player> player = std::nullopt)
{
    uint64_t bishops = player.value_or(board.player) == Player::White ? board.pieces[WhiteBishop] : board.pieces[BlackBishop];
    const uint64_t occupancy = board.occupation[Both];

    uint64_t attacks = {};
    while (bishops) {
        const int from = std::countr_zero(bishops);
        bishops &= bishops - 1;

        attacks |= movement::getBishopAttacks(from, occupancy);
    }

    return attacks;
}

constexpr static inline uint64_t getQueenAttacks(const BitBoard& board, std::optional<Player> player = std::nullopt)
{
    uint64_t queens = player.value_or(board.player) == Player::White ? board.pieces[WhiteQueen] : board.pieces[BlackQueen];
    const uint64_t occupancy = board.occupation[Both];

    uint64_t attacks = {};
    while (queens) {
        const int from = std::countr_zero(queens);
        queens &= queens - 1;

        attacks |= movement::getRookAttacks(from, occupancy);
        attacks |= movement::getBishopAttacks(from, occupancy);
    }

    return attacks;
}

constexpr static inline uint64_t getKingAttacks(const BitBoard& board, std::optional<Player> player = std::nullopt)
{
    const uint64_t king = player.value_or(board.player) == Player::White ? board.pieces[WhiteKing] : board.pieces[BlackKing];

    if (king == 0) {
        return 0;
    }

    const int from = std::countr_zero(king);
    return movement::s_kingsTable.at(from);
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
    uint64_t attacks = ((pawns & ~s_aFileMask) >> 7);
    attacks |= ((pawns & ~s_hFileMask) >> 9);

    return attacks;
}

constexpr uint64_t getAllAttacks(const BitBoard& board, Player player)
{
    uint64_t attacks = player == Player::White
        ? gen::getWhitePawnAttacks(board)
        : gen::getBlackPawnAttacks(board);

    attacks |= gen::getKnightAttacks(board, player);
    attacks |= gen::getRookAttacks(board, player);
    attacks |= gen::getBishopAttacks(board, player);
    attacks |= gen::getQueenAttacks(board, player);
    attacks |= gen::getKingAttacks(board, player);

    return attacks;
}

}
