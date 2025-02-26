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

std::string enPessantToString(std::optional<uint64_t> val)
{
    if (val.has_value() && val.value() != 0) {
        const int position = std::countr_zero(val.value());

        char column = 'a' + (position % 8);
        char row = '1' + (position / 8);

        return std::string { column, row };
    }

    return "none";
}

constexpr static inline void clearPiece(uint64_t& piece, uint64_t square)
{
    piece &= ~square;
}

constexpr static inline void movePiece(uint64_t& piece, uint64_t fromSquare, uint64_t toSquare)
{
    piece &= ~fromSquare;
    piece |= toSquare;
}

constexpr static inline void performCastleMove(BitBoard& board, const movement::Move& move)
{
    const auto fromSquare = move.fromSquare();
    const auto toSquare = move.toSquare();

    switch (move.castleType()) {
    case CastleWhiteKingSide: {
        movePiece(board.pieces[WhiteKing], fromSquare, toSquare);
        movePiece(board.pieces[WhiteRook], 1ULL << gen::s_whiteKingSideCastleMoveRook.first, 1ULL << gen::s_whiteKingSideCastleMoveRook.second);
        board.castlingRights &= ~(CastleWhiteKingSide | CastleWhiteQueenSide);
    } break;

    case CastleWhiteQueenSide: {
        movePiece(board.pieces[WhiteKing], fromSquare, toSquare);
        movePiece(board.pieces[WhiteRook], 1ULL << gen::s_whiteQueenSideCastleMoveRook.first, 1ULL << gen::s_whiteQueenSideCastleMoveRook.second);
        board.castlingRights &= ~(CastleWhiteKingSide | CastleWhiteQueenSide);
    } break;
    case CastleBlackKingSide: {
        movePiece(board.pieces[BlackKing], fromSquare, toSquare);
        movePiece(board.pieces[BlackRook], 1ULL << gen::s_blackKingSideCastleMoveRook.first, 1ULL << gen::s_blackKingSideCastleMoveRook.second);
        board.castlingRights &= ~(CastleBlackKingSide | CastleBlackQueenSide);
    } break;
    case CastleBlackQueenSide: {
        movePiece(board.pieces[BlackKing], fromSquare, toSquare);
        movePiece(board.pieces[BlackRook], 1ULL << gen::s_blackQueenSideCastleMoveRook.first, 1ULL << gen::s_blackQueenSideCastleMoveRook.second);
        board.castlingRights &= ~(CastleBlackKingSide | CastleBlackQueenSide);
    } break;
    case CastleNone:
        break;
    }
}

constexpr static inline void performPromotionMove(BitBoard& board, const movement::Move& move)
{
    const bool isWhite = board.player == Player::White;

    // first clear to be promoted pawn
    uint64_t& pawns = isWhite ? board.pieces[WhitePawn] : board.pieces[BlackPawn];
    pawns &= ~move.fromSquare();

    const auto findPromotionPiece = [&] -> uint64_t& {
        switch (move.promotionType()) {
        case PromotionQueen:
        case PromotionNone:
            return isWhite ? board.pieces[WhiteQueen] : board.pieces[BlackQueen];
        case PromotionKnight:
            return isWhite ? board.pieces[WhiteKnight] : board.pieces[BlackKnight];
        case PromotionBishop:
            return isWhite ? board.pieces[WhiteBishop] : board.pieces[BlackBishop];
        case PromotionRook:
            return isWhite ? board.pieces[WhiteRook] : board.pieces[BlackRook];
        }

        return isWhite ? board.pieces[WhiteQueen] : board.pieces[BlackQueen];
    };

    // replace pawn with given promotion type
    uint64_t& promotionPiece = findPromotionPiece();
    promotionPiece |= move.fromSquare();
}

constexpr static inline void updateCastlingRights(BitBoard& board, uint64_t square)
{
    constexpr uint64_t whiteKingSquare = { 1ULL << 4 };
    constexpr uint64_t whiteRookQueenSideSquare = { 1ULL << 0 };
    constexpr uint64_t whiteRookKingSideSquare = { 1ULL << 7 };

    constexpr uint64_t blackKingSquare = { whiteKingSquare << s_eightRow };
    constexpr uint64_t blackRookQueenSideSquare = { whiteRookQueenSideSquare << s_eightRow };
    constexpr uint64_t blackRookKingSideSquare = { whiteRookKingSideSquare << s_eightRow };

    if (board.castlingRights == CastleNone) {
        return;
    }

    switch (square) {
    case whiteKingSquare:
        board.castlingRights &= ~(CastleWhiteKingSide | CastleWhiteQueenSide);
        break;
    case whiteRookQueenSideSquare:
        board.castlingRights &= ~CastleWhiteQueenSide;
        break;
    case whiteRookKingSideSquare:
        board.castlingRights &= ~CastleWhiteKingSide;
        break;
    case blackKingSquare:
        board.castlingRights &= ~(CastleBlackKingSide | CastleBlackQueenSide);
        break;
    case blackRookQueenSideSquare:
        board.castlingRights &= ~CastleBlackQueenSide;
        break;
    case blackRookKingSideSquare:
        board.castlingRights &= ~CastleBlackKingSide;
        break;
    }
}

}

constexpr static inline movement::ValidMoves getAllMoves(const BitBoard& board)
{
    movement::ValidMoves validMoves;

    if (board.player == Player::White) {
        uint64_t attacks = gen::getAllAttacks(board, Player::Black);
        gen::getKingMoves(validMoves, board, attacks);
        gen::getPawnMoves(validMoves, board);
        gen::getKnightMoves(validMoves, board);
        gen::getRookMoves(validMoves, board);
        gen::getBishopMoves(validMoves, board);
        gen::getQueenMoves(validMoves, board);
        gen::getCastlingMoves(validMoves, board, attacks);
    } else {
        uint64_t attacks = gen::getAllAttacks(board, Player::White);
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

constexpr static inline movement::ValidMoves getAllCaptures(const BitBoard& board)
{
    movement::ValidMoves captures;
    const auto allMoves = getAllMoves(board);

    for (const auto& move : allMoves.getMoves()) {
        if (move.isCapture())
            captures.addMove(move);
    }

    return captures;
}

constexpr static inline bool isKingAttacked(const BitBoard& board, Player player)
{
    if (player == Player::White) {
        return board.pieces[WhiteKing] & gen::getAllAttacks(board, Player::Black);
    } else {
        return board.pieces[BlackKing] & gen::getAllAttacks(board, Player::White);
    }
}

constexpr static inline bool isKingAttacked(const BitBoard& board)
{
    if (board.player == Player::White) {
        return board.pieces[WhiteKing] & gen::getAllAttacks(board, Player::Black);
    } else {
        return board.pieces[BlackKing] & gen::getAllAttacks(board, Player::White);
    }
}

constexpr static inline BitBoard performMove(const BitBoard& board, const movement::Move& move)
{
    BitBoard newBoard = board;

    const auto fromSquare = move.fromSquare();
    const auto toSquare = move.toSquare();

    if (move.isCastleMove()) {
        performCastleMove(newBoard, move);
    } else if (move.takeEnPessant()) {
        if (newBoard.player == Player::White) {
            movePiece(newBoard.pieces[WhitePawn], fromSquare, toSquare);
            newBoard.pieces[BlackPawn] &= ~(toSquare >> 8);
        } else {
            movePiece(newBoard.pieces[BlackPawn], fromSquare, toSquare);
            newBoard.pieces[WhitePawn] &= ~(toSquare << 8);
        }
    } else {
        if (move.isPromotionMove())
            performPromotionMove(newBoard, move);

        // should be done before moving pieces
        updateCastlingRights(newBoard, fromSquare);
        updateCastlingRights(newBoard, toSquare);

        for (const auto piece : magic_enum::enum_values<Piece>()) {
            if (board.pieces[piece] & toSquare) {
                clearPiece(newBoard.pieces[piece], toSquare);
                break;
            }
        }

        const auto piece = newBoard.getPieceAtSquare(fromSquare);
        if (piece.has_value()) {
            movePiece(newBoard.pieces[piece.value()], fromSquare, toSquare);
        }
    }

    if (move.hasEnPessant()) {
        newBoard.enPessant = move.piece() == Piece::WhitePawn ? move.toSquare() >> 8 : move.toSquare() << 8;
    } else {
        newBoard.enPessant.reset();
    }

    newBoard.updateOccupation();

    /* player making the move is black -> inc full moves */
    if (board.player == Player::Black)
        newBoard.fullMoves++;

    if (move.isCapture())
        newBoard.halfMoves = 0;
    else
        newBoard.halfMoves++;

    newBoard.player = nextPlayer(newBoard.player);

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

constexpr static inline void printBoardDebug(FileLogger& logger, const BitBoard& board)
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
    logger << "\nEnPessant: " << enPessantToString(board.enPessant);
    logger << "\nHash: " << std::to_string(generateHashKey(board));
    logger << "\nCastle: ";
    for (const auto& move : allMoves.getMoves()) {
        if (move.isCastleMove()) {
            logger << move.toString().data() << " ";
        }
    }

    logger << "\n\nMoves[" << allMoves.count() << "]:\n";
    for (const auto& move : allMoves.getMoves()) {
        logger << move.toString().data() << " ";
    }

    logger << "\n";
    logger.flush();
}

}
