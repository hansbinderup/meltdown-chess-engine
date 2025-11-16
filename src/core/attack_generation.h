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
    const uint64_t knights = board.pieces[Knight] & board.occupation[player];

    uint64_t attacks = {};
    utils::bitIterate(knights, [&](BoardPosition from) {
        attacks |= movegen::getKnightMoves(from);
    });

    return attacks;
}

template<Player player>
constexpr static inline uint64_t getRookAttacks(const BitBoard& board)
{
    const uint64_t rooks = board.pieces[Rook] & board.occupation[player];
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
    const uint64_t bishops = board.pieces[Bishop] & board.occupation[player];
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
    const uint64_t queens = board.pieces[Queen] & board.occupation[player];
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
    const uint64_t king = board.pieces[King] & board.occupation[player];

    if (king == 0) {
        return 0;
    }

    return movegen::getKingMoves(utils::lsbToPosition(king));
}

template<Player player>
constexpr static inline uint64_t getPawnAttacks(const BitBoard& board)
{
    if constexpr (player == PlayerWhite) {
        const uint64_t pawns = board.pieces[Pawn] & board.occupation[PlayerWhite];
        uint64_t attacks = ((pawns & ~s_aFileMask) << 7);
        attacks |= ((pawns & ~s_hFileMask) << 9);

        return attacks;
    } else {
        const uint64_t pawns = board.pieces[Pawn] & board.occupation[PlayerBlack];
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

/* computes discovered attacks based on the given position
 * NOTE: queen attacks are not included - it's only rook and bishops */
template<Player player>
constexpr uint64_t getDiscoveredAttacks(const BitBoard& board, BoardPosition pos)
{
    constexpr Player opponent = nextPlayer(player);

    const uint64_t occupancy = board.occupation[Both];

    const uint64_t rookAttacks = movegen::getRookMoves(pos, occupancy);
    const uint64_t bishopAttacks = movegen::getBishopMoves(pos, occupancy);

    const uint64_t rooks = board.pieces[Rook] & board.occupation[opponent] & ~rookAttacks;
    const uint64_t bishops = board.pieces[Bishop] & board.occupation[opponent] & ~bishopAttacks;

    return (rooks & movegen::getRookMoves(pos, occupancy & ~rookAttacks))
        | (bishops & movegen::getBishopMoves(pos, occupancy & ~bishopAttacks));
}

}
