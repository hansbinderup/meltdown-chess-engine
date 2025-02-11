#pragma once

#include "bishops.h"
#include "kings.h"
#include "knights.h"
#include "pawns.h"
#include "rooks.h"

#include "src/bit_board.h"
#include "src/board_defs.h"

namespace gen {

constexpr movement::Move s_whiteKingSideCastleMove { 4, 6, movement::MoveFlags::Castle };
constexpr movement::Move s_whiteQueenSideCastleMove { 4, 2, movement::MoveFlags::Castle };
constexpr movement::Move s_whiteKingSideCastleMoveRook { 7, 5 };
constexpr movement::Move s_whiteQueenSideCastleMoveRook { 0, 3 };

constexpr movement::Move s_blackKingSideCastleMove { 4 + s_eightRow, 6 + s_eightRow, movement::MoveFlags::Castle };
constexpr movement::Move s_blackQueenSideCastleMove { 4 + s_eightRow, 2 + s_eightRow, movement::MoveFlags::Castle };
constexpr movement::Move s_blackKingSideCastleMoveRook { 7 + s_eightRow, 5 + s_eightRow };
constexpr movement::Move s_blackQueenSideCastleMoveRook { 0 + s_eightRow, 3 + s_eightRow };

namespace {

constexpr static inline void generateKnightMoves(movement::ValidMoves& validMoves, uint64_t knights, uint64_t ownOccupation, uint64_t theirOccupation)
{
    while (knights) {
        const int from = std::countr_zero(knights);
        knights &= knights - 1;

        uint64_t moves = movement::s_knightsTable.at(from) & ~ownOccupation;

        while (moves) {
            const int to = std::countr_zero(moves);
            const movement::MoveFlags flags = (1ULL << to) & theirOccupation ? movement::MoveFlags::Capture : movement::MoveFlags::None;
            moves &= moves - 1;

            validMoves.addMove({ static_cast<uint8_t>(from), static_cast<uint8_t>(to), flags });
        }
    }
}

constexpr static inline void generateRookMoves(movement::ValidMoves& validMoves, uint64_t rooks, uint64_t ownOccupation, uint64_t theirOccupation)
{
    while (rooks) {
        const int from = std::countr_zero(rooks);
        rooks &= rooks - 1;

        uint64_t moves = movement::getRookAttacks(from, ownOccupation | theirOccupation) & ~ownOccupation;

        while (moves) {
            const int to = std::countr_zero(moves);
            const movement::MoveFlags flags = (1ULL << to) & theirOccupation ? movement::MoveFlags::Capture : movement::MoveFlags::None;
            moves &= moves - 1;

            validMoves.addMove({ static_cast<uint8_t>(from), static_cast<uint8_t>(to), flags });
        }
    }
}

constexpr static inline void generateBishopMoves(movement::ValidMoves& validMoves, uint64_t bishops, uint64_t ownOccupation, uint64_t theirOccupation)
{
    while (bishops) {
        const int from = std::countr_zero(bishops);
        bishops &= bishops - 1;

        uint64_t moves = movement::getBishopAttacks(from, ownOccupation | theirOccupation) & ~ownOccupation;

        while (moves) {
            const int to = std::countr_zero(moves);
            const movement::MoveFlags flags = (1ULL << to) & theirOccupation ? movement::MoveFlags::Capture : movement::MoveFlags::None;
            moves &= moves - 1;

            validMoves.addMove({ static_cast<uint8_t>(from), static_cast<uint8_t>(to), flags });
        }
    }
}

constexpr static inline void generateQueenMoves(movement::ValidMoves& validMoves, uint64_t queens, uint64_t ownOccupation, uint64_t theirOccupation)
{
    generateRookMoves(validMoves, queens, ownOccupation, theirOccupation);
    generateBishopMoves(validMoves, queens, ownOccupation, theirOccupation);
}

constexpr static inline void generateKingMoves(movement::ValidMoves& validMoves, uint64_t king, uint64_t ownOccupation, uint64_t theirOccupation, uint64_t attacks)
{
    if (king == 0)
        return;

    const int from = std::countr_zero(king);
    king &= king - 1;

    uint64_t moves = movement::s_kingsTable.at(from) & ~ownOccupation & ~attacks;

    while (moves) {
        const int to = std::countr_zero(moves);
        const movement::MoveFlags flags = (1ULL << to) & theirOccupation ? movement::MoveFlags::Capture : movement::MoveFlags::None;
        moves &= moves - 1;

        validMoves.addMove({ static_cast<uint8_t>(from), static_cast<uint8_t>(to), flags });
    }
}

}

constexpr static inline void getKnightMoves(movement::ValidMoves& validMoves, const BitBoard& board)
{
    generateKnightMoves(
        validMoves,
        board.player == Player::White ? board.whiteKnights : board.blackKnights,
        board.player == Player::White ? board.getWhiteOccupation() : board.getBlackOccupation(),
        board.player == Player::White ? board.getBlackOccupation() : board.getWhiteOccupation());
}

constexpr static inline void getRookMoves(movement::ValidMoves& validMoves, const BitBoard& board)
{
    generateRookMoves(
        validMoves,
        board.player == Player::White ? board.whiteRooks : board.blackRooks,
        board.player == Player::White ? board.getWhiteOccupation() : board.getBlackOccupation(),
        board.player == Player::White ? board.getBlackOccupation() : board.getWhiteOccupation());
}

constexpr static inline void getBishopMoves(movement::ValidMoves& validMoves, const BitBoard& board)
{
    generateBishopMoves(
        validMoves,
        board.player == Player::White ? board.whiteBishops : board.blackBishops,
        board.player == Player::White ? board.getWhiteOccupation() : board.getBlackOccupation(),
        board.player == Player::White ? board.getBlackOccupation() : board.getWhiteOccupation());
}

constexpr static inline void getQueenMoves(movement::ValidMoves& validMoves, const BitBoard& board)
{
    getRookMoves(validMoves, board);
    getBishopMoves(validMoves, board);
}

constexpr static inline void getKingMoves(movement::ValidMoves& validMoves, const BitBoard& board, uint64_t attacks)
{
    generateKingMoves(
        validMoves,
        board.player == Player::White ? board.whiteKing : board.blackKing,
        board.player == Player::White ? board.getWhiteOccupation() : board.getBlackOccupation(),
        board.player == Player::White ? board.getBlackOccupation() : board.getWhiteOccupation(),
        attacks);
}

constexpr static inline void getPawnMoves(movement::ValidMoves& validMoves, const BitBoard& board)
{
    if (board.player == Player::White) {
        movement::getWhitePawnMoves(validMoves, board.whitePawns, board.getWhiteOccupation(), board.getBlackOccupation());
    } else {
        movement::getBlackPawnMoves(validMoves, board.blackPawns, board.getBlackOccupation(), board.getWhiteOccupation());
    }
}

constexpr static inline void getCastlingMoves(movement::ValidMoves& validMoves, const BitBoard& board, uint64_t attacks)
{
    const uint64_t castlingMask = board.player == Player::White ? board.whiteCastlingRights : board.blackCastlingRights;
    const uint64_t king = board.player == Player::White ? board.whiteKing : board.blackKing;
    const uint64_t rook = board.player == Player::White ? board.whiteRooks : board.blackRooks;
    const uint64_t occupation = board.getAllOccupation();

    if (castlingMask == 0 || king == 0 || rook == 0) {
        return;
    }

    if (attacks & king) {
        return;
    }

    const uint64_t kingRookMask = king | rook;
    const uint64_t queenSideCastleMask = board.player == Player::White ? s_whiteQueenSideCastleMask : s_blackQueenSideCastleMask;
    const uint64_t kingSideCastleMask = board.player == Player::White ? s_whiteKingSideCastleMask : s_blackKingSideCastleMask;
    const uint64_t queenSideCastleOccupationMask = board.player == Player::White ? s_whiteQueenSideCastleOccupationMask : s_blackQueenSideCastleOccupationMask;
    const uint64_t kingSideCastleOccupationMask = board.player == Player::White ? s_whiteKingSideCastleOccupationMask : s_blackKingSideCastleOccupationMask;
    const uint64_t queenSideCastleAttackMask = board.player == Player::White ? s_whiteQueenSideCastleAttackMask : s_blackQueenSideCastleAttackMask;
    const uint64_t kingSideCastleAttackMask = board.player == Player::White ? s_whiteKingSideCastleAttackMask : s_blackKingSideCastleAttackMask;
    const movement::Move queenSideCastleMove = board.player == Player::White ? s_whiteQueenSideCastleMove : s_blackQueenSideCastleMove;
    const movement::Move kingSideCastleMove = board.player == Player::White ? s_whiteKingSideCastleMove : s_blackKingSideCastleMove;

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
