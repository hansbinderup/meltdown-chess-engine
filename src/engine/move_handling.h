#pragma once

#include "attack_generation.h"
#include "bit_board.h"
#include "engine/tt_hash_table.h"
#include "engine/zobrist_hashing.h"
#include "helpers/formatters.h"
#include "movegen/move_generation.h"
#include "movegen/move_types.h"
#include "parsing/piece_parsing.h"

#include "fmt/ranges.h"
#include "magic_enum/magic_enum.hpp"

#include <cstring>

namespace engine {

namespace {

constexpr static inline void clearPiece(uint64_t& piece, BoardPosition pos, Piece type, uint64_t& hash)
{
    piece &= ~helper::positionToSquare(pos);
    engine::hashPiece(type, pos, hash); // remove from hash
}

constexpr static inline void setPiece(uint64_t& piece, BoardPosition pos, Piece type, uint64_t& hash)
{
    piece |= helper::positionToSquare(pos);
    engine::hashPiece(type, pos, hash); // add to hash
}

constexpr static inline void movePiece(uint64_t& piece, BoardPosition fromPos, BoardPosition toPos, Piece type, uint64_t& hash)
{
    clearPiece(piece, fromPos, type, hash);
    setPiece(piece, toPos, type, hash);
}

constexpr void performCastleMove(BitBoard& board, const movegen::Move& move, uint64_t& hash)
{
    const BoardPosition fromPos = move.fromPos();
    const BoardPosition toPos = move.toPos();

    switch (move.castleType(board.player)) {
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

template<Player player>
constexpr void performPromotionMove(BitBoard& board, const movegen::Move& move, uint64_t& hash)
{
    constexpr bool isWhite = player == PlayerWhite;
    constexpr auto type = isWhite ? WhitePawn : BlackPawn;

    // first clear to be promoted pawn
    uint64_t& pawns = board.pieces[type];
    clearPiece(pawns, move.fromPos(), type, hash);

    /* clear piece that will be taken if capture */
    if (move.isCapture()) {
        if (const auto victim = board.getTargetAtSquare<player>(move.toSquare())) {
            clearPiece(board.pieces[victim.value()], move.toPos(), victim.value(), hash);
        }
    }

    switch (move.promotionType()) {
    case PromotionNone:
        return;
    case PromotionQueen: {
        constexpr auto type = isWhite ? WhiteQueen : BlackQueen;
        setPiece(board.pieces[type], move.toPos(), type, hash);
    } break;
    case PromotionKnight: {
        constexpr auto type = isWhite ? WhiteKnight : BlackKnight;
        setPiece(board.pieces[type], move.toPos(), type, hash);
    } break;
    case PromotionBishop: {
        constexpr auto type = isWhite ? WhiteBishop : BlackBishop;
        setPiece(board.pieces[type], move.toPos(), type, hash);
    } break;
    case PromotionRook: {
        constexpr auto type = isWhite ? WhiteRook : BlackRook;
        setPiece(board.pieces[type], move.toPos(), type, hash);
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

template<movegen::MoveType type>
constexpr void getAllMoves(const BitBoard& board, movegen::ValidMoves& moves)
{
    if (board.player == PlayerWhite) {
        const uint64_t attacks = board.attacks[PlayerBlack];
        movegen::getKingMoves<PlayerWhite, type>(moves, board, attacks);
        movegen::getPawnMoves<PlayerWhite, type>(moves, board);
        movegen::getKnightMoves<PlayerWhite, type>(moves, board);
        movegen::getRookMoves<PlayerWhite, type>(moves, board);
        movegen::getBishopMoves<PlayerWhite, type>(moves, board);
        movegen::getQueenMoves<PlayerWhite, type>(moves, board);
        movegen::getCastlingMoves<PlayerWhite, type>(moves, board, attacks);
    } else {
        const uint64_t attacks = board.attacks[PlayerWhite];
        movegen::getKingMoves<PlayerBlack, type>(moves, board, attacks);
        movegen::getPawnMoves<PlayerBlack, type>(moves, board);
        movegen::getKnightMoves<PlayerBlack, type>(moves, board);
        movegen::getRookMoves<PlayerBlack, type>(moves, board);
        movegen::getBishopMoves<PlayerBlack, type>(moves, board);
        movegen::getQueenMoves<PlayerBlack, type>(moves, board);
        movegen::getCastlingMoves<PlayerBlack, type>(moves, board, attacks);
    }
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

template<Player player>
constexpr static inline BoardPosition enpessantCapturePosition(BoardPosition pos)
{
    if constexpr (player == PlayerWhite) {
        return intToBoardPosition(pos - 8);
    } else {
        return intToBoardPosition(pos + 8);
    }
}

template<Player player>
constexpr BitBoard performMove(const BitBoard& board, const movegen::Move& move, uint64_t& hash)
{
    BitBoard newBoard = board;

    const auto fromPos = move.fromPos();
    const auto toPos = move.toPos();
    const auto pieceType = board.getAttackerAtSquare<player>(move.fromSquare()).value();

    if (move.isCastleMove()) {
        performCastleMove(newBoard, move, hash);
    } else if (move.takeEnPessant()) {
        if constexpr (player == PlayerWhite) {
            movePiece(newBoard.pieces[WhitePawn], fromPos, toPos, WhitePawn, hash);
            clearPiece(newBoard.pieces[BlackPawn], enpessantCapturePosition<player>(toPos), BlackPawn, hash);
        } else {
            movePiece(newBoard.pieces[BlackPawn], fromPos, toPos, BlackPawn, hash);
            clearPiece(newBoard.pieces[WhitePawn], enpessantCapturePosition<player>(toPos), WhitePawn, hash);
        }
    } else if (move.isPromotionMove()) {
        performPromotionMove<player>(newBoard, move, hash);
    } else {
        if (move.isCapture()) {
            if (const auto victim = board.getTargetAtSquare<player>(move.toSquare())) {
                clearPiece(newBoard.pieces[victim.value()], move.toPos(), victim.value(), hash);
            }
        }

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

    if (move.isDoublePush()) {
        newBoard.enPessant = enpessantCapturePosition<player>(toPos);
        hashEnpessant(newBoard.enPessant.value(), hash);
    }

    newBoard.updateOccupation();

    newBoard.attacks[PlayerWhite] = attackgen::getAllAttacks<PlayerWhite>(newBoard);
    newBoard.attacks[PlayerBlack] = attackgen::getAllAttacks<PlayerBlack>(newBoard);

    /* player making the move is black -> inc full moves */
    if constexpr (player == PlayerBlack)
        newBoard.fullMoves++;

    if (move.isCapture() || pieceType == WhitePawn || pieceType == BlackPawn)
        newBoard.halfMoves = 0;
    else
        newBoard.halfMoves++;

    newBoard.player = nextPlayer(player);
    hashPlayer(hash);

    /* prefetch as soon as we have calculated the key/hash */
    engine::TtHashTable::prefetch(hash);

    return newBoard;
}

// Helper: using inside loops means redundant colour checks
constexpr BitBoard performMove(const BitBoard& board, const movegen::Move& move, uint64_t& hash)
{
    if (board.player == PlayerWhite)
        return performMove<PlayerWhite>(board, move, hash);
    else
        return performMove<PlayerBlack>(board, move, hash);
}

constexpr void printPositionDebug(const BitBoard& board)
{
    fmt::println("");

    for (uint8_t row = 8; row > 0; row--) {
        fmt::print("{} ", row);

        for (uint8_t column = 0; column < 8; column++) {
            const auto pos = intToBoardPosition((row - 1) * 8 + column);
            uint64_t square = helper::positionToSquare(pos);
            auto attacker = board.getAttackerAtSquare<PlayerWhite>(square);
            if (!attacker.has_value()) {
                attacker = board.getAttackerAtSquare<PlayerBlack>(square);
            }
            fmt::print("{} ", attacker ? parsing::pieceToUnicode(*attacker) : "·");
        }

        fmt::print("\n");
    }

    fmt::print("  A B C D E F G H\n\n");

    movegen::ValidMoves moves;
    getAllMoves<movegen::MovePseudoLegal>(board, moves);

    fmt::print(
        "Player: {}\n"
        "FullMoves: {}\n"
        "HalfMoves: {}\n"
        "EnPessant: {}\n"
        "Hash: {}\n",
        board.player,
        board.fullMoves,
        board.halfMoves,
        board.enPessant ? magic_enum::enum_name(*board.enPessant) : "none",
        generateHashKey(board));

    // Print castling moves
    fmt::print("Castle: ");
    for (const auto& move : moves) {
        if (move.isCastleMove()) {
            fmt::print("{} ", move);
        }
    }

    fmt::println("\nMoves[{}]: {}\n", moves.count(), fmt::join(moves, ", "));
}

}
