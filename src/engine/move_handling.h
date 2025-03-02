#pragma once

#include "magic_enum/magic_enum.hpp"
#include "src/attack_generation.h"
#include "src/bit_board.h"
#include "src/engine/zobrist_hashing.h"
#include "src/file_logger.h"
#include "src/movement/move_generation.h"
#include "src/movement/move_types.h"
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

constexpr movement::ValidMoves getAllMoves(const BitBoard& board)
{
    movement::ValidMoves validMoves;

    if (board.player == PlayerWhite) {
        uint64_t attacks = gen::getAllAttacks(board, PlayerBlack);
        gen::getKingMoves(validMoves, board, attacks);
        gen::getPawnMoves(validMoves, board);
        gen::getKnightMoves(validMoves, board);
        gen::getRookMoves(validMoves, board);
        gen::getBishopMoves(validMoves, board);
        gen::getQueenMoves(validMoves, board);
        gen::getCastlingMoves(validMoves, board, attacks);
    } else {
        uint64_t attacks = gen::getAllAttacks(board, PlayerWhite);
        gen::getKingMoves(validMoves, board, attacks);
        gen::getPawnMoves(validMoves, board);
        gen::getKnightMoves(validMoves, board);
        gen::getRookMoves(validMoves, board);
        gen::getBishopMoves(validMoves, board);
        gen::getQueenMoves(validMoves, board);
        gen::getCastlingMoves(validMoves, board, attacks);
    }

    return validMoves;
}

constexpr movement::ValidMoves getAllCaptures(const BitBoard& board)
{
    movement::ValidMoves captures;
    const auto allMoves = getAllMoves(board);

    for (const auto& move : allMoves) {
        if (move.isCapture())
            captures.addMove(move);
    }

    return captures;
}

constexpr static inline bool isKingAttacked(const BitBoard& board, Player player)
{
    if (player == PlayerWhite) {
        return board.pieces[WhiteKing] & gen::getAllAttacks(board, PlayerBlack);
    } else {
        return board.pieces[BlackKing] & gen::getAllAttacks(board, PlayerWhite);
    }
}

constexpr static inline bool isKingAttacked(const BitBoard& board)
{
    if (board.player == PlayerWhite) {
        return board.pieces[WhiteKing] & gen::getAllAttacks(board, PlayerBlack);
    } else {
        return board.pieces[BlackKing] & gen::getAllAttacks(board, PlayerWhite);
    }
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
        /* TODO: add this check - not sure how well it is for now? */
        /* if (move.isCapture()) { */
        if (const auto victim = board.getPieceAtSquare(move.toSquare())) {
            clearPiece(newBoard.pieces[victim.value()], move.toValue(), victim.value(), hash);
        }
        /* } */

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

constexpr static inline std::string pieceToStr(Piece piece)
{
    switch (piece) {
    case WhitePawn:
        return "WP";
    case WhiteKnight:
        return "WN";
    case WhiteBishop:
        return "WB";
    case WhiteRook:
        return "WR";
    case WhiteQueen:
        return "WQ";
    case WhiteKing:
        return "WK";
    case BlackPawn:
        return "BP";
    case BlackKnight:
        return "BN";
    case BlackBishop:
        return "BB";
    case BlackRook:
        return "BR";
    case BlackQueen:
        return "BQ";
    case BlackKing:
        return "BK";
    }

    return "##";
}

constexpr void printBoardDebug(FileLogger& logger, const BitBoard& board)
{
    logger.log("\n******* BitBoard debug: *******\n");

    for (uint8_t row = 8; row > 0; row--) {
        logger << std::format("-{}- ", row);

        for (uint8_t column = 0; column < 8; column++) {
            uint64_t square = 1ULL << (((row - 1) * 8) + column);
            const auto piece = board.getPieceAtSquare(square);
            if (piece.has_value()) {
                logger << pieceToStr(piece.value()) << " ";
            } else {
                logger << "## ";
            }
        }

        logger << "\n";
    }

    logger << "    A  B  C  D  E  F  G  H";
    logger << "\n\n";

    const auto allMoves = getAllMoves(board);
    logger << "Player: " << magic_enum::enum_name(board.player);
    logger << "\nFullMoves: " << std::to_string(board.fullMoves);
    logger << "\nHalfMoves: " << std::to_string(board.halfMoves);
    logger << "\nEnPessant: " << (board.enPessant.has_value() ? magic_enum::enum_name(board.enPessant.value()) : "none");
    logger << "\nHash: " << std::to_string(generateHashKey(board));
    logger << "\nCastle: ";
    for (const auto& move : allMoves) {
        if (move.isCastleMove()) {
            logger << move.toString().data() << " ";
        }
    }

    logger << "\n\nMoves[" << allMoves.count() << "]:\n";
    for (const auto& move : allMoves) {
        logger << move.toString().data() << " ";
    }

    logger << "\n";
    logger.flush();
}
}
