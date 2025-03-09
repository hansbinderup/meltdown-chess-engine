#pragma once

#include "attack_generation.h"
#include "bit_board.h"
#include "engine/zobrist_hashing.h"
#include "fmt/ranges.h"
#include "magic_enum/magic_enum.hpp"
#include "movement/move_generation.h"
#include "movement/move_types.h"
#include "parsing/piece_parsing.h"
#include <cstring>

namespace engine {

namespace {

constexpr static inline void clearPiece(uint64_t& piece, BoardPosition pos, Piece type, uint64_t& hash)
{
    piece &= ~(1ULL << pos);
    engine::hashPiece(type, pos, hash); // remove from hash
}

constexpr static inline void setPiece(uint64_t& piece, BoardPosition pos, Piece type, uint64_t& hash)
{
    piece |= (1ULL << pos);
    engine::hashPiece(type, pos, hash); // add to hash
}

constexpr static inline void movePiece(uint64_t& piece, BoardPosition fromPos, BoardPosition toPos, Piece type, uint64_t& hash)
{
    clearPiece(piece, fromPos, type, hash);
    setPiece(piece, toPos, type, hash);
}

constexpr void performCastleMove(BitBoard& board, const movement::Move& move, uint64_t& hash)
{
    const BoardPosition fromPos = move.fromValue();
    const BoardPosition toPos = move.toValue();

    switch (move.castleType()) {
    case CastleWhiteKingSide: {
        movePiece(board.pieces[WhiteKing], fromPos, toPos, WhiteKing, hash);
        movePiece(board.pieces[WhiteRook], H1, F1, WhiteRook, hash);
    } break;

    case CastleWhiteQueenSide: {
        movePiece(board.pieces[WhiteKing], fromPos, toPos, WhiteKing, hash);
        movePiece(board.pieces[WhiteRook], A1, D1, WhiteRook, hash);
    } break;
    case CastleBlackKingSide: {
        movePiece(board.pieces[BlackKing], fromPos, toPos, BlackKing, hash);
        movePiece(board.pieces[BlackRook], H8, F8, BlackRook, hash);
    } break;
    case CastleBlackQueenSide: {
        movePiece(board.pieces[BlackKing], fromPos, toPos, BlackKing, hash);
        movePiece(board.pieces[BlackRook], A8, D8, BlackRook, hash);
    } break;
    case CastleNone:
        break;
    }
}

constexpr void performPromotionMove(BitBoard& board, const movement::Move& move, uint64_t& hash)
{
    const bool isWhite = board.player == PlayerWhite;
    const auto type = isWhite ? WhitePawn : BlackPawn;

    // first clear to be promoted pawn
    uint64_t& pawns = isWhite ? board.pieces[WhitePawn] : board.pieces[BlackPawn];
    clearPiece(pawns, move.fromValue(), type, hash);

    /* clear piece that will be taken if capture */
    if (move.isCapture()) {
        if (const auto victim = board.getPieceAtSquare(move.toSquare())) {
            clearPiece(board.pieces[victim.value()], move.toValue(), victim.value(), hash);
        }
    }

    switch (move.promotionType()) {
    case PromotionNone:
        return;
    case PromotionQueen: {
        const auto type = isWhite ? WhiteQueen : BlackQueen;
        setPiece(board.pieces[type], move.toValue(), type, hash);
    } break;
    case PromotionKnight: {
        const auto type = isWhite ? WhiteKnight : BlackKnight;
        setPiece(board.pieces[type], move.toValue(), type, hash);
    } break;
    case PromotionBishop: {
        const auto type = isWhite ? WhiteBishop : BlackBishop;
        setPiece(board.pieces[type], move.toValue(), type, hash);
    } break;
    case PromotionRook: {
        const auto type = isWhite ? WhiteRook : BlackRook;
        setPiece(board.pieces[type], move.toValue(), type, hash);
    } break;
    }
}

constexpr void updateCastlingRights(BitBoard& board, BoardPosition pos)
{
    if (board.castlingRights == CastleNone) {
        return;
    }

    switch (pos) {
    case E1:
        board.castlingRights &= ~(CastleWhiteKingSide | CastleWhiteQueenSide);
        break;
    case A1:
        board.castlingRights &= ~CastleWhiteQueenSide;
        break;
    case H1:
        board.castlingRights &= ~CastleWhiteKingSide;
        break;
    case E8:
        board.castlingRights &= ~(CastleBlackKingSide | CastleBlackQueenSide);
        break;
    case A8:
        board.castlingRights &= ~CastleBlackQueenSide;
        break;
    case H8:
        board.castlingRights &= ~CastleBlackKingSide;
        break;
    default:
        break;
    }
}

}

template<movement::MoveType type>
constexpr movement::ValidMoves getAllMoves(const BitBoard& board)
{
    movement::ValidMoves validMoves;

    if (board.player == PlayerWhite) {
        const uint64_t attacks = board.attacks[PlayerBlack];
        gen::getKingMoves<PlayerWhite, type>(validMoves, board, attacks);
        gen::getPawnMoves<PlayerWhite, type>(validMoves, board);
        gen::getKnightMoves<PlayerWhite, type>(validMoves, board);
        gen::getRookMoves<PlayerWhite, type>(validMoves, board);
        gen::getBishopMoves<PlayerWhite, type>(validMoves, board);
        gen::getQueenMoves<PlayerWhite, type>(validMoves, board);
        gen::getCastlingMoves<PlayerWhite, type>(validMoves, board, attacks);
    } else {
        const uint64_t attacks = board.attacks[PlayerWhite];
        gen::getKingMoves<PlayerBlack, type>(validMoves, board, attacks);
        gen::getPawnMoves<PlayerBlack, type>(validMoves, board);
        gen::getKnightMoves<PlayerBlack, type>(validMoves, board);
        gen::getRookMoves<PlayerBlack, type>(validMoves, board);
        gen::getBishopMoves<PlayerBlack, type>(validMoves, board);
        gen::getQueenMoves<PlayerBlack, type>(validMoves, board);
        gen::getCastlingMoves<PlayerBlack, type>(validMoves, board, attacks);
    }

    return validMoves;
}

constexpr static inline bool isKingAttacked(const BitBoard& board, Player player)
{
    if (player == PlayerWhite) {
        return board.pieces[WhiteKing] & board.attacks[PlayerBlack];
    } else {
        return board.pieces[BlackKing] & board.attacks[PlayerWhite];
    }
}

constexpr static inline bool isKingAttacked(const BitBoard& board)
{
    return isKingAttacked(board, board.player);
}

constexpr static inline BoardPosition enpessantCapturePosition(BoardPosition pos, Player player)
{
    if (player == PlayerWhite) {
        return static_cast<BoardPosition>(pos - 8);
    } else {
        return static_cast<BoardPosition>(pos + 8);
    }
}

constexpr BitBoard performMove(const BitBoard& board, const movement::Move& move, uint64_t& hash)
{
    BitBoard newBoard = board;

    const auto fromPos = move.fromValue();
    const auto toPos = move.toValue();

    if (move.isCastleMove()) {
        performCastleMove(newBoard, move, hash);
    } else if (move.takeEnPessant()) {
        if (newBoard.player == PlayerWhite) {
            movePiece(newBoard.pieces[WhitePawn], fromPos, toPos, WhitePawn, hash);
            clearPiece(newBoard.pieces[BlackPawn], enpessantCapturePosition(toPos, PlayerWhite), BlackPawn, hash);
        } else {
            movePiece(newBoard.pieces[BlackPawn], fromPos, toPos, BlackPawn, hash);
            clearPiece(newBoard.pieces[WhitePawn], enpessantCapturePosition(toPos, PlayerBlack), WhitePawn, hash);
        }
    } else if (move.isPromotionMove()) {
        performPromotionMove(newBoard, move, hash);
    } else {
        if (move.isCapture()) {
            if (const auto victim = board.getPieceAtSquare(move.toSquare())) {
                clearPiece(newBoard.pieces[victim.value()], move.toValue(), victim.value(), hash);
            }
        }

        const auto pieceType = move.piece();
        movePiece(newBoard.pieces[pieceType], fromPos, toPos, pieceType, hash);
    }

    hashCastling(newBoard.castlingRights, hash); // remove current hash
    updateCastlingRights(newBoard, fromPos);
    updateCastlingRights(newBoard, toPos);
    hashCastling(newBoard.castlingRights, hash); // add new hash

    if (board.enPessant.has_value()) {
        newBoard.enPessant.reset();
        hashEnpessant(board.enPessant.value(), hash);
    }

    if (move.hasEnPessant()) {
        newBoard.enPessant = enpessantCapturePosition(toPos, newBoard.player);
        hashEnpessant(newBoard.enPessant.value(), hash);
    }

    newBoard.updateOccupation();

    newBoard.attacks[PlayerWhite] = gen::getAllAttacks<PlayerWhite>(newBoard);
    newBoard.attacks[PlayerBlack] = gen::getAllAttacks<PlayerBlack>(newBoard);

    /* player making the move is black -> inc full moves */
    if (board.player == PlayerBlack)
        newBoard.fullMoves++;

    if (move.isCapture())
        newBoard.halfMoves = 0;
    else
        newBoard.halfMoves++;

    newBoard.player = nextPlayer(newBoard.player);
    hashPlayer(hash);

    return newBoard;
}

constexpr void printPositionDebug(const BitBoard& board)
{
    fmt::println("");

    for (uint8_t row = 8; row > 0; row--) {
        fmt::print("{} ", row);

        for (uint8_t column = 0; column < 8; column++) {
            uint64_t square = 1ULL << (((row - 1) * 8) + column);
            const auto piece = board.getPieceAtSquare(square);
            fmt::print("{} ", piece ? parsing::pieceToUnicode(*piece) : "·");
        }

        fmt::print("\n");
    }

    fmt::print("  A B C D E F G H\n\n");

    const auto allMoves = getAllMoves<movement::MovePseudoLegal>(board);

    fmt::print(
        "Player: {}\n"
        "FullMoves: {}\n"
        "HalfMoves: {}\n"
        "EnPessant: {}\n"
        "Hash: {}\n",
        magic_enum::enum_name(board.player),
        board.fullMoves,
        board.halfMoves,
        board.enPessant ? magic_enum::enum_name(*board.enPessant) : "none",
        generateHashKey(board));

    // Print castling moves
    fmt::print("Castle: ");
    for (const auto& move : allMoves) {
        if (move.isCastleMove()) {
            fmt::print("{} ", move);
        }
    }

    fmt::println("\nMoves[{}]: {}\n", allMoves.count(), fmt::join(allMoves, ", "));
}

}
