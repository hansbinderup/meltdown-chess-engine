#pragma once

#include "bishops.h"
#include "helpers/bit_operations.h"
#include "kings.h"
#include "knights.h"
#include "pawns.h"
#include "rooks.h"

#include "bit_board.h"
#include "board_defs.h"

namespace gen {

namespace {

template<movement::MoveType type>
constexpr static inline void generateKnightMoves(movement::ValidMoves& validMoves, uint64_t knights, uint64_t ownOccupation, uint64_t theirOccupation, Piece piece)
{
    helper::bitIterate(knights, [&](BoardPosition from) {
        uint64_t moves = movement::s_knightsTable.at(from) & ~ownOccupation;

        helper::bitIterate(moves, [&](BoardPosition pos) {
            const bool isCapture = (1ULL << pos) & theirOccupation;
            if constexpr (type == movement::MovePseudoLegal) {
                validMoves.addMove(movement::Move::create(from, pos, piece, isCapture));
            } else if constexpr (type == movement::MoveCapture) {
                if (isCapture)
                    validMoves.addMove(movement::Move::create(from, pos, piece, isCapture));
            }
        });
    });
}

template<movement::MoveType type>
constexpr static inline void generateRookMoves(movement::ValidMoves& validMoves, uint64_t rooks, uint64_t ownOccupation, uint64_t theirOccupation, Piece piece)
{
    helper::bitIterate(rooks, [&](BoardPosition from) {
        uint64_t moves = movement::getRookAttacks(from, ownOccupation | theirOccupation) & ~ownOccupation;

        helper::bitIterate(moves, [&](BoardPosition to) {
            const bool isCapture = (1ULL << to) & theirOccupation;

            if constexpr (type == movement::MovePseudoLegal) {
                validMoves.addMove(movement::Move::create(from, to, piece, isCapture));
            } else if constexpr (type == movement::MoveCapture) {
                if (isCapture)
                    validMoves.addMove(movement::Move::create(from, to, piece, isCapture));
            }
        });
    });
}

template<movement::MoveType type>
constexpr static inline void generateBishopMoves(movement::ValidMoves& validMoves, uint64_t bishops, uint64_t ownOccupation, uint64_t theirOccupation, Piece piece)
{
    helper::bitIterate(bishops, [&](BoardPosition from) {
        uint64_t moves = movement::getBishopAttacks(from, ownOccupation | theirOccupation) & ~ownOccupation;

        helper::bitIterate(moves, [&](BoardPosition to) {
            const bool isCapture = (1ULL << to) & theirOccupation;

            if constexpr (type == movement::MovePseudoLegal) {
                validMoves.addMove(movement::Move::create(from, to, piece, isCapture));
            } else if constexpr (type == movement::MoveCapture) {
                if (isCapture)
                    validMoves.addMove(movement::Move::create(from, to, piece, isCapture));
            }
        });
    });
}

template<movement::MoveType type>
constexpr static inline void generateQueenMoves(movement::ValidMoves& validMoves, uint64_t queens, uint64_t ownOccupation, uint64_t theirOccupation, Piece piece)
{
    generateRookMoves<type>(validMoves, queens, ownOccupation, theirOccupation, piece);
    generateBishopMoves<type>(validMoves, queens, ownOccupation, theirOccupation, piece);
}

template<movement::MoveType type>
constexpr static inline void generateKingMoves(movement::ValidMoves& validMoves, uint64_t king, uint64_t ownOccupation, uint64_t theirOccupation, Piece piece, uint64_t attacks)
{
    helper::bitIterate(king, [&](BoardPosition from) {
        uint64_t moves = movement::s_kingsTable.at(from) & ~ownOccupation & ~attacks;

        helper::bitIterate(moves, [&](BoardPosition to) {
            const bool isCapture = (1ULL << to) & theirOccupation;

            if constexpr (type == movement::MovePseudoLegal) {
                validMoves.addMove(movement::Move::create(from, to, piece, isCapture));
            } else if constexpr (type == movement::MoveCapture) {
                if (isCapture)
                    validMoves.addMove(movement::Move::create(from, to, piece, isCapture));
            }
        });
    });
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

template<Player player, movement::MoveType type>
constexpr static inline void getKnightMoves(movement::ValidMoves& validMoves, const BitBoard& board)
{
    if constexpr (player == PlayerWhite) {
        generateKnightMoves<type>(validMoves, board.pieces[WhiteKnight], board.occupation[White], board.occupation[Black], Piece::WhiteKnight);
    } else {
        generateKnightMoves<type>(validMoves, board.pieces[BlackKnight], board.occupation[Black], board.occupation[White], Piece::BlackKnight);
    }
}

template<Player player, movement::MoveType type>
constexpr static inline void getRookMoves(movement::ValidMoves& validMoves, const BitBoard& board)
{
    if constexpr (player == PlayerWhite) {
        generateRookMoves<type>(validMoves, board.pieces[WhiteRook], board.occupation[White], board.occupation[Black], Piece::WhiteRook);
    } else {
        generateRookMoves<type>(validMoves, board.pieces[BlackRook], board.occupation[Black], board.occupation[White], Piece::BlackRook);
    }
}

template<Player player, movement::MoveType type>
constexpr static inline void getBishopMoves(movement::ValidMoves& validMoves, const BitBoard& board)
{
    if constexpr (player == PlayerWhite) {
        generateBishopMoves<type>(validMoves, board.pieces[WhiteBishop], board.occupation[White], board.occupation[Black], Piece::WhiteBishop);
    } else {
        generateBishopMoves<type>(validMoves, board.pieces[BlackBishop], board.occupation[Black], board.occupation[White], Piece::BlackBishop);
    }
}

template<Player player, movement::MoveType type>
constexpr static inline void getQueenMoves(movement::ValidMoves& validMoves, const BitBoard& board)
{
    if constexpr (player == PlayerWhite) {
        generateQueenMoves<type>(validMoves, board.pieces[WhiteQueen], board.occupation[White], board.occupation[Black], Piece::WhiteQueen);
    } else {
        generateQueenMoves<type>(validMoves, board.pieces[BlackQueen], board.occupation[Black], board.occupation[White], Piece::BlackQueen);
    }
}

template<Player player, movement::MoveType type>
constexpr static inline void getKingMoves(movement::ValidMoves& validMoves, const BitBoard& board, uint64_t attacks)
{
    if constexpr (player == PlayerWhite) {
        generateKingMoves<type>(validMoves, board.pieces[WhiteKing], board.occupation[White], board.occupation[Black], Piece::WhiteKing, attacks);
    } else {
        generateKingMoves<type>(validMoves, board.pieces[BlackKing], board.occupation[Black], board.occupation[White], Piece::BlackKing, attacks);
    }
}

template<Player player, movement::MoveType type>
constexpr static inline void getPawnMoves(movement::ValidMoves& validMoves, const BitBoard& board)
{
    if constexpr (player == PlayerWhite) {
        movement::getWhitePawnMoves<type>(validMoves, board.pieces[WhitePawn], board.occupation[White], board.occupation[Black]);
        if (board.enPessant.has_value()) {
            movement::getWhiteEnPessantMoves(validMoves, board.pieces[WhitePawn], board.enPessant.value(), board.occupation[Both]);
        }
    } else {
        movement::getBlackPawnMoves<type>(validMoves, board.pieces[BlackPawn], board.occupation[Black], board.occupation[White]);
        if (board.enPessant.has_value()) {
            movement::getBlackEnPessantMoves(validMoves, board.pieces[BlackPawn], board.enPessant.value(), board.occupation[Both]);
        }
    }
}

template<Player player, movement::MoveType type>
constexpr static inline void getCastlingMoves(movement::ValidMoves& validMoves, const BitBoard& board, uint64_t attacks)
{
    if constexpr (type == movement::MoveCapture) {
        return;
    }

    if constexpr (player == PlayerWhite) {
        generateCastlingMovesWhite(validMoves, board, attacks);
    } else {
        generateCastlingMovesBlack(validMoves, board, attacks);
    }
}
}
