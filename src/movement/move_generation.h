#pragma once

#include "bishops.h"
#include "kings.h"
#include "knights.h"
#include "pawns.h"
#include "rooks.h"

#include "src/board_defs.h"
#include <iostream>

namespace gen {

constexpr static inline void getPawnMoves(movement::ValidMoves& validMoves, Player player, uint64_t pawns, uint64_t whiteOccupation, uint64_t blackOccupation)
{
    if (player == Player::White) {
        movement::getWhitePawnMoves(validMoves, pawns, whiteOccupation, blackOccupation);
    } else {
        movement::getBlackPawnMoves(validMoves, pawns, whiteOccupation, blackOccupation);
    }
}

constexpr static inline void getKnightMoves(movement::ValidMoves& validMoves, uint64_t knights, uint64_t ownOccupation)
{
    while (knights) {
        const int from = std::countr_zero(knights);
        knights &= knights - 1;

        uint64_t moves = movement::s_knightsTable.at(from) & ~ownOccupation;

        while (moves) {
            int to = std::countr_zero(moves);
            moves &= moves - 1;
            validMoves.addMove({ static_cast<uint8_t>(from), static_cast<uint8_t>(to) });
        }
    }
}

constexpr static inline void getRookMoves(movement::ValidMoves& validMoves, uint64_t rooks, uint64_t ownOccupation, uint64_t theirOccupation)
{
    while (rooks) {
        const int from = std::countr_zero(rooks);
        rooks &= rooks - 1;

        uint64_t moves = movement::getRookAttacks(from, ownOccupation | theirOccupation) & ~ownOccupation;

        while (moves) {
            int to = std::countr_zero(moves);
            moves &= moves - 1;
            validMoves.addMove({ static_cast<uint8_t>(from), static_cast<uint8_t>(to) });
        }
    }
}

constexpr static inline void getBishopMoves(movement::ValidMoves& validMoves, uint64_t bishops, uint64_t ownOccupation, uint64_t theirOccupation)
{
    while (bishops) {
        const int from = std::countr_zero(bishops);
        bishops &= bishops - 1;

        uint64_t moves = movement::getBishopAttacks(from, ownOccupation | theirOccupation) & ~ownOccupation;

        while (moves) {
            int to = std::countr_zero(moves);
            moves &= moves - 1;
            validMoves.addMove({ static_cast<uint8_t>(from), static_cast<uint8_t>(to) });
        }
    }
}

constexpr static inline void getQueenMoves(movement::ValidMoves& validMoves, uint64_t queens, uint64_t ownOccupation, uint64_t theirOccupation)
{
    getRookMoves(validMoves, queens, ownOccupation, theirOccupation);
    getBishopMoves(validMoves, queens, ownOccupation, theirOccupation);
}

constexpr static inline void getKingMoves(movement::ValidMoves& validMoves, uint64_t king, uint64_t ownOccupation, uint64_t attacks)
{
    if (king == 0)
        return;

    const int from = std::countr_zero(king);
    king &= king - 1;

    uint64_t moves = movement::s_kingsTable.at(from) & ~ownOccupation & ~attacks;

    while (moves) {
        int to = std::countr_zero(moves);
        moves &= moves - 1;
        validMoves.addMove({ static_cast<uint8_t>(from), static_cast<uint8_t>(to) });
    }
}

}
