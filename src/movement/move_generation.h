#pragma once

#include "bishops.h"
#include "kings.h"
#include "knights.h"
#include "pawns.h"
#include "rooks.h"

#include "src/board_defs.h"

namespace gen {

constexpr movement::Move s_whiteKingSideCastleMove { 4, 6 };
constexpr movement::Move s_whiteQueenSideCastleMove { 4, 2 };
constexpr movement::Move s_whiteKingSideCastleMoveRook { 7, 5 };
constexpr movement::Move s_whiteQueenSideCastleMoveRook { 0, 3 };

constexpr movement::Move s_blackKingSideCastleMove { 4 + s_eightRow, 6 + s_eightRow };
constexpr movement::Move s_blackQueenSideCastleMove { 4 + s_eightRow, 2 + s_eightRow };
constexpr movement::Move s_blackKingSideCastleMoveRook { 7 + s_eightRow, 5 + s_eightRow };
constexpr movement::Move s_blackQueenSideCastleMoveRook { 0 + s_eightRow, 3 + s_eightRow };

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

constexpr static inline void getCastlingMoves(movement::ValidMoves& validMoves, Player player, uint64_t king, uint64_t rook, uint64_t castlingMask, uint64_t occupation, uint64_t attacks)
{
    if (castlingMask == 0 || king == 0 || rook == 0) {
        return;
    }

    if (attacks & king) {
        return;
    }

    const uint64_t kingRookMask = king | rook;
    const uint64_t queenSideCastleMask = player == Player::White ? s_whiteQueenSideCastleMask : s_blackQueenSideCastleMask;
    const uint64_t kingSideCastleMask = player == Player::White ? s_whiteKingSideCastleMask : s_blackKingSideCastleMask;
    const uint64_t queenSideCastleOccupationMask = player == Player::White ? s_whiteQueenSideCastleOccupationMask : s_blackQueenSideCastleOccupationMask;
    const uint64_t kingSideCastleOccupationMask = player == Player::White ? s_whiteKingSideCastleOccupationMask : s_blackKingSideCastleOccupationMask;
    const uint64_t queenSideCastleAttackMask = player == Player::White ? s_whiteQueenSideCastleAttackMask : s_blackQueenSideCastleAttackMask;
    const uint64_t kingSideCastleAttackMask = player == Player::White ? s_whiteKingSideCastleAttackMask : s_blackKingSideCastleAttackMask;
    const movement::Move queenSideCastleMove = player == Player::White ? s_whiteQueenSideCastleMove : s_blackQueenSideCastleMove;
    const movement::Move kingSideCastleMove = player == Player::White ? s_whiteKingSideCastleMove : s_blackKingSideCastleMove;

    // Queen side castling
    if ((castlingMask & queenSideCastleMask & kingRookMask) == queenSideCastleMask) {
        // Pieces in the way - castling not allowed
        if (!(occupation & queenSideCastleOccupationMask) && !(attacks & queenSideCastleAttackMask)) {
            validMoves.addMove(queenSideCastleMove);
        }
    }

    // King side castling
    if ((castlingMask & kingSideCastleMask & kingRookMask) == kingSideCastleMask) {
        // Pieces in the way - castling not allowed
        if (!(occupation & kingSideCastleOccupationMask) && !(attacks & kingSideCastleAttackMask)) {
            validMoves.addMove(kingSideCastleMove);
        }
    }
}

}
