#pragma once

#include "bishops.h"
#include "kings.h"
#include "knights.h"
#include "pawns.h"
#include "rooks.h"

#include "src/bit_board.h"
#include "src/board_defs.h"

namespace gen {

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

            validMoves.addMove(movement::Move::create(from, to, piece, isCapture));
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

            validMoves.addMove(movement::Move::create(from, to, piece, isCapture));
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

            validMoves.addMove(movement::Move::create(from, to, piece, isCapture));
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

        validMoves.addMove(movement::Move::create(from, to, piece, isCapture));
    }
}

constexpr static inline void generateCastlingMovesWhite(movement::ValidMoves& validMoves, const BitBoard& board, uint64_t attacks)
{
    constexpr uint64_t queenSideAttackMask { 0x1cULL }; // 0b11100 - fields that can block castling if attacked
    constexpr uint64_t kingSideAttackMask { 0x30ULL }; // 0b110000 - fields that can block castling if attacked
    constexpr uint64_t queenSideOccupationMask { 0xeULL }; // 0b1110 - fields that can block castling if occupied
    constexpr uint64_t kingSideOccupationMask { 0x60ULL }; // 0b1100000 - fields that can block castling if occupied

    const uint64_t occupation = board.occupation[Both];

    // Queen side castling
    if (board.castlingRights & CastleWhiteQueenSide) {
        // Pieces in the way - castling not allowed
        if (!(occupation & queenSideOccupationMask) && !(attacks & queenSideAttackMask)) {
            validMoves.addMove(movement::Move::createCastle(E1, C1, WhiteKing, CastleWhiteQueenSide));
        }
    }

    // King side castling
    if (board.castlingRights & CastleWhiteKingSide) {
        // Pieces in the way - castling not allowed
        if (!(occupation & kingSideOccupationMask) && !(attacks & kingSideAttackMask)) {
            validMoves.addMove(movement::Move::createCastle(E1, G1, WhiteKing, CastleWhiteKingSide));
        }
    }
}

constexpr static inline void generateCastlingMovesBlack(movement::ValidMoves& validMoves, const BitBoard& board, uint64_t attacks)
{
    constexpr uint64_t queenSideAttackMask { 0x1cULL << s_eightRow };
    constexpr uint64_t kingSideAttackMask { 0x30ULL << s_eightRow };
    constexpr uint64_t queenSideOccupationMask { 0xeULL << s_eightRow };
    constexpr uint64_t kingSideOccupationMask { 0x60ULL << s_eightRow };

    const uint64_t occupation = board.occupation[Both];

    // Queen side castling
    if (board.castlingRights & CastleBlackQueenSide) {
        // Pieces in the way - castling not allowed
        if (!(occupation & queenSideOccupationMask) && !(attacks & queenSideAttackMask)) {
            validMoves.addMove(movement::Move::createCastle(E8, C8, BlackKing, CastleBlackQueenSide));
        }
    }

    // King side castling
    if (board.castlingRights & CastleBlackKingSide) {
        // Pieces in the way - castling not allowed
        if (!(occupation & kingSideOccupationMask) && !(attacks & kingSideAttackMask)) {
            validMoves.addMove(movement::Move::createCastle(E8, G8, BlackKing, CastleBlackKingSide));
        }
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
            movement::getWhiteEnPessantMoves(validMoves, board.pieces[WhitePawn], board.enPessant.value(), board.occupation[Both]);
        }
    } else {
        movement::getBlackPawnMoves(validMoves, board.pieces[BlackPawn], board.occupation[Black], board.occupation[White]);
        if (board.enPessant.has_value()) {
            movement::getBlackEnPessantMoves(validMoves, board.pieces[BlackPawn], board.enPessant.value(), board.occupation[Both]);
        }
    }
}

constexpr static inline void getCastlingMoves(movement::ValidMoves& validMoves, const BitBoard& board, uint64_t attacks)
{
    if (board.player == Player::White) {
        generateCastlingMovesWhite(validMoves, board, attacks);
    } else {
        generateCastlingMovesBlack(validMoves, board, attacks);
    }
}
}
