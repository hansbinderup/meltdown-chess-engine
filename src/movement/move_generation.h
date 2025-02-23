#pragma once

#include "bishops.h"
#include "kings.h"
#include "knights.h"
#include "pawns.h"
#include "rooks.h"

#include "src/bit_board.h"
#include "src/board_defs.h"

namespace gen {

using CastleMove = std::pair<uint8_t, uint8_t>;

constexpr CastleMove s_whiteKingSideCastleMove { 4, 6 };
constexpr CastleMove s_whiteQueenSideCastleMove { 4, 2 };

constexpr CastleMove s_whiteKingSideCastleMoveRook { 7, 5 };
constexpr CastleMove s_whiteQueenSideCastleMoveRook { 0, 3 };

constexpr CastleMove s_blackKingSideCastleMove { 4 + s_eightRow, 6 + s_eightRow };
constexpr CastleMove s_blackQueenSideCastleMove { 4 + s_eightRow, 2 + s_eightRow };

constexpr CastleMove s_blackKingSideCastleMoveRook { 7 + s_eightRow, 5 + s_eightRow };
constexpr CastleMove s_blackQueenSideCastleMoveRook { 0 + s_eightRow, 3 + s_eightRow };

namespace {
constexpr static inline void generateKnightMoves(movement::ValidMoves& validMoves, uint64_t knights, uint64_t ownOccupation, uint64_t theirOccupation, Piece piece)
{
    while (knights) {
        const int from = std::countr_zero(knights);
        knights &= knights - 1;

        uint64_t moves = movement::s_knightsTable.at(from) & ~ownOccupation;

        while (moves) {
            const int to = std::countr_zero(moves);
            const bool isCapture = (1ULL << to) & theirOccupation;
            moves &= moves - 1;

            validMoves.addMove(movement::Move::create(static_cast<uint8_t>(from), static_cast<uint8_t>(to), piece, isCapture));
        }
    }
}

constexpr static inline void generateRookMoves(movement::ValidMoves& validMoves, uint64_t rooks, uint64_t ownOccupation, uint64_t theirOccupation, Piece piece)
{
    while (rooks) {
        const int from = std::countr_zero(rooks);
        rooks &= rooks - 1;

        uint64_t moves = movement::getRookAttacks(from, ownOccupation | theirOccupation) & ~ownOccupation;

        while (moves) {
            const int to = std::countr_zero(moves);
            const bool isCapture = (1ULL << to) & theirOccupation;
            moves &= moves - 1;

            validMoves.addMove(movement::Move::create(static_cast<uint8_t>(from), static_cast<uint8_t>(to), piece, isCapture));
        }
    }
}

constexpr static inline void generateBishopMoves(movement::ValidMoves& validMoves, uint64_t bishops, uint64_t ownOccupation, uint64_t theirOccupation, Piece piece)
{
    while (bishops) {
        const int from = std::countr_zero(bishops);
        bishops &= bishops - 1;

        uint64_t moves = movement::getBishopAttacks(from, ownOccupation | theirOccupation) & ~ownOccupation;

        while (moves) {
            const int to = std::countr_zero(moves);
            const bool isCapture = (1ULL << to) & theirOccupation;
            moves &= moves - 1;

            validMoves.addMove(movement::Move::create(static_cast<uint8_t>(from), static_cast<uint8_t>(to), piece, isCapture));
        }
    }
}

constexpr static inline void generateQueenMoves(movement::ValidMoves& validMoves, uint64_t queens, uint64_t ownOccupation, uint64_t theirOccupation, Piece piece)
{
    generateRookMoves(validMoves, queens, ownOccupation, theirOccupation, piece);
    generateBishopMoves(validMoves, queens, ownOccupation, theirOccupation, piece);
}

constexpr static inline void generateKingMoves(movement::ValidMoves& validMoves, uint64_t king, uint64_t ownOccupation, uint64_t theirOccupation, Piece piece, uint64_t attacks)
{
    if (king == 0)
        return;

    const int from = std::countr_zero(king);
    king &= king - 1;

    uint64_t moves = movement::s_kingsTable.at(from) & ~ownOccupation & ~attacks;

    while (moves) {
        const int to = std::countr_zero(moves);
        const bool isCapture = (1ULL << to) & theirOccupation;
        moves &= moves - 1;

        validMoves.addMove(movement::Move::create(static_cast<uint8_t>(from), static_cast<uint8_t>(to), piece, isCapture));
    }
}

}

constexpr static inline void getKnightMoves(movement::ValidMoves& validMoves, const BitBoard& board)
{
    generateKnightMoves(
        validMoves,
        board.player == Player::White ? board.pieces[WhiteKnight] : board.pieces[BlackKnight],
        board.player == Player::White ? board.occupation[White] : board.occupation[Black],
        board.player == Player::White ? board.occupation[Black] : board.occupation[White],
        board.player == Player::White ? Piece::WhiteKnight : Piece::BlackKnight);
}

constexpr static inline void getRookMoves(movement::ValidMoves& validMoves, const BitBoard& board)
{
    generateRookMoves(
        validMoves,
        board.player == Player::White ? board.pieces[WhiteRook] : board.pieces[BlackRook],
        board.player == Player::White ? board.occupation[White] : board.occupation[Black],
        board.player == Player::White ? board.occupation[Black] : board.occupation[White],
        board.player == Player::White ? Piece::WhiteRook : Piece::BlackRook);
}

constexpr static inline void getBishopMoves(movement::ValidMoves& validMoves, const BitBoard& board)
{
    generateBishopMoves(
        validMoves,
        board.player == Player::White ? board.pieces[WhiteBishop] : board.pieces[BlackBishop],
        board.player == Player::White ? board.occupation[White] : board.occupation[Black],
        board.player == Player::White ? board.occupation[Black] : board.occupation[White],
        board.player == Player::White ? Piece::WhiteBishop : Piece::BlackBishop);
}

constexpr static inline void getQueenMoves(movement::ValidMoves& validMoves, const BitBoard& board)
{
    generateQueenMoves(
        validMoves,
        board.player == Player::White ? board.pieces[WhiteQueen] : board.pieces[BlackQueen],
        board.player == Player::White ? board.occupation[White] : board.occupation[Black],
        board.player == Player::White ? board.occupation[Black] : board.occupation[White],
        board.player == Player::White ? Piece::WhiteQueen : Piece::BlackQueen);
}

constexpr static inline void getKingMoves(movement::ValidMoves& validMoves, const BitBoard& board, uint64_t attacks)
{
    generateKingMoves(
        validMoves,
        board.player == Player::White ? board.pieces[WhiteKing] : board.pieces[BlackKing],
        board.player == Player::White ? board.occupation[White] : board.occupation[Black],
        board.player == Player::White ? board.occupation[Black] : board.occupation[White],
        board.player == Player::White ? Piece::WhiteKing : Piece::BlackKing,
        attacks);
}

constexpr static inline void getPawnMoves(movement::ValidMoves& validMoves, const BitBoard& board)
{
    if (board.player == Player::White) {
        movement::getWhitePawnMoves(validMoves, board.pieces[WhitePawn], board.occupation[White], board.occupation[Black]);
        if (board.enPessant.has_value()) {
            movement::getWhiteEnPessantMoves(validMoves, board.pieces[WhitePawn], board.enPessant.value(), board.occupation[White], board.occupation[Black]);
        }
    } else {
        movement::getBlackPawnMoves(validMoves, board.pieces[BlackPawn], board.occupation[Black], board.occupation[White]);
        if (board.enPessant.has_value()) {
            movement::getWhiteEnPessantMoves(validMoves, board.pieces[BlackPawn], board.enPessant.value(), board.occupation[Black], board.occupation[White]);
        }
    }
}

// TODO: separate in player - this is not super pretty
constexpr static inline void getCastlingMoves(movement::ValidMoves& validMoves, const BitBoard& board, uint64_t attacks)
{
    const bool isWhite = board.player == Player::White;
    const uint64_t castlingMask = isWhite ? board.whiteCastlingRights : board.blackCastlingRights;
    const uint64_t king = isWhite ? board.pieces[WhiteKing] : board.pieces[BlackKing];
    const uint64_t rook = isWhite ? board.pieces[WhiteRook] : board.pieces[BlackRook];
    const uint64_t occupation = board.occupation[Both];

    if (castlingMask == 0 || king == 0 || rook == 0) {
        return;
    }

    if (attacks & king) {
        return;
    }

    Piece piece = isWhite ? Piece::WhiteKing : Piece::BlackKing;
    const uint64_t kingRookMask = king | rook;
    const uint64_t queenSideCastleMask = isWhite ? s_whiteQueenSideCastleMask : s_blackQueenSideCastleMask;
    const uint64_t kingSideCastleMask = isWhite ? s_whiteKingSideCastleMask : s_blackKingSideCastleMask;
    const uint64_t queenSideCastleOccupationMask = isWhite ? s_whiteQueenSideCastleOccupationMask : s_blackQueenSideCastleOccupationMask;
    const uint64_t kingSideCastleOccupationMask = isWhite ? s_whiteKingSideCastleOccupationMask : s_blackKingSideCastleOccupationMask;
    const uint64_t queenSideCastleAttackMask = isWhite ? s_whiteQueenSideCastleAttackMask : s_blackQueenSideCastleAttackMask;
    const uint64_t kingSideCastleAttackMask = isWhite ? s_whiteKingSideCastleAttackMask : s_blackKingSideCastleAttackMask;
    const auto queenSideCastleMove = isWhite ? s_whiteQueenSideCastleMove : s_blackQueenSideCastleMove;
    const auto kingSideCastleMove = isWhite ? s_whiteKingSideCastleMove : s_blackKingSideCastleMove;

    // Queen side castling
    if ((castlingMask & queenSideCastleMask & kingRookMask) == queenSideCastleMask) {
        // Pieces in the way - castling not allowed
        if (!(occupation & queenSideCastleOccupationMask) && !(attacks & queenSideCastleAttackMask)) {
            validMoves.addMove(movement::Move::createCastle(queenSideCastleMove.first, queenSideCastleMove.second, piece, isWhite ? CastleType::WhiteQueenSide : CastleType::BlackQueenSide));
        }
    }

    // King side castling
    if ((castlingMask & kingSideCastleMask & kingRookMask) == kingSideCastleMask) {
        // Pieces in the way - castling not allowed
        if (!(occupation & kingSideCastleOccupationMask) && !(attacks & kingSideCastleAttackMask)) {
            validMoves.addMove(
                movement::Move::createCastle(kingSideCastleMove.first, kingSideCastleMove.second, piece, isWhite ? CastleType::WhiteKingSide : CastleType::BlackKingSide));
        }
    }
}

}
