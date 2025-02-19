#pragma once

#include "magic_enum/magic_enum.hpp"
#include "src/attack_generation.h"
#include "src/bit_board.h"
#include "src/evaluation/move_scoring.h"
#include "src/file_logger.h"
#include "src/movement/move_generation.h"
#include "src/movement/move_types.h"

namespace engine {

namespace {

constexpr static inline void bitToggleMove(uint64_t& piece, uint64_t fromSquare, uint64_t toSquare)
{
    if (toSquare & piece) {
        // clear piece if being attacked
        piece &= ~toSquare;
    } else if (fromSquare & piece) {
        // move if given piece is located at "fromSquare"
        piece &= ~fromSquare;
        piece |= toSquare;
    }
}

constexpr static inline void performCastleMove(BitBoard& board, const movement::Move& move)
{
    const auto fromSquare = move.fromSquare();
    const auto toSquare = move.toSquare();

    switch (move.castleType()) {
    case CastleType::WhiteKingSide: {
        bitToggleMove(board.whiteKing, fromSquare, toSquare);
        bitToggleMove(board.whiteRooks, 1ULL << gen::s_whiteKingSideCastleMoveRook.first, 1ULL << gen::s_whiteKingSideCastleMoveRook.second);
        board.whiteCastlingRights = 0;
    } break;

    case CastleType::WhiteQueenSide: {
        bitToggleMove(board.whiteKing, fromSquare, toSquare);
        bitToggleMove(board.whiteRooks, 1ULL << gen::s_whiteQueenSideCastleMoveRook.first, 1ULL << gen::s_whiteQueenSideCastleMoveRook.second);
        board.whiteCastlingRights = 0;
    } break;
    case CastleType::BlackKingSide: {
        bitToggleMove(board.blackKing, fromSquare, toSquare);
        bitToggleMove(board.blackRooks, 1ULL << gen::s_blackKingSideCastleMoveRook.first, 1ULL << gen::s_blackKingSideCastleMoveRook.second);
        board.blackCastlingRights = 0;
    } break;
    case CastleType::BlackQueenSide: {
        bitToggleMove(board.blackKing, fromSquare, toSquare);
        bitToggleMove(board.blackRooks, 1ULL << gen::s_blackQueenSideCastleMoveRook.first, 1ULL << gen::s_blackQueenSideCastleMoveRook.second);
        board.blackCastlingRights = 0;
    } break;
    case CastleType::None:
        break;
    }
}

constexpr static inline void performPromotionMove(BitBoard& board, const movement::Move& move)
{
    const bool isWhite = board.player == Player::White;

    // first clear to be promoted pawn
    uint64_t& pawns = isWhite ? board.whitePawns : board.blackPawns;
    pawns &= ~move.fromSquare();

    const auto findPromotionPiece = [&] -> uint64_t& {
        switch (move.promotionType()) {
        case PromotionType::Queen:
        case PromotionType::None:
            return isWhite ? board.whiteQueens : board.blackQueens;
        case PromotionType::Knight:
            return isWhite ? board.whiteKnights : board.blackKnights;
        case PromotionType::Bishop:
            return isWhite ? board.whiteBishops : board.blackBishops;
        case PromotionType::Rook:
            return isWhite ? board.whiteRooks : board.blackRooks;
        }

        return isWhite ? board.whiteQueens : board.blackQueens;
    };

    // replace pawn with given promotion type
    uint64_t& promotionPiece = findPromotionPiece();
    promotionPiece |= move.fromSquare();
}

constexpr static inline void updateCastlingRights(BitBoard& board, uint64_t fromSquare)
{
    if (board.player == Player::White && board.whiteCastlingRights) {
        if (fromSquare & board.whiteKing) {
            board.whiteCastlingRights = 0;
        } else {
            board.whiteCastlingRights &= ~fromSquare;
        }
    } else if (board.player == Player::Black && board.blackCastlingRights) {
        if (fromSquare & board.blackKing) {
            board.blackCastlingRights = 0;
        } else {
            board.blackCastlingRights &= ~fromSquare;
        }
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

constexpr static inline movement::ValidMoves getAllMovesSorted(const BitBoard& board, uint8_t ply)
{
    auto allMoves = getAllMoves(board);
    std::sort(allMoves.getMoves().begin(), allMoves.getMoves().end(), [&](const auto& a, const auto& b) {
        return evaluation::MoveScoring::score(board, a, ply) > evaluation::MoveScoring::score(board, b, ply);
    });

    return allMoves;
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

constexpr static inline movement::ValidMoves getAllCapturesSorted(const BitBoard& board, uint8_t ply)
{
    auto captures = getAllCaptures(board);
    std::sort(captures.getMoves().begin(), captures.getMoves().end(), [&](const auto& a, const auto& b) {
        return evaluation::MoveScoring::score(board, a, ply) > evaluation::MoveScoring::score(board, b, ply);
    });

    return captures;
}

constexpr static inline bool isKingAttacked(const BitBoard& board, Player player)
{
    if (player == Player::White) {
        return board.whiteKing & gen::getAllAttacks(board, Player::Black);
    } else {
        return board.blackKing & gen::getAllAttacks(board, Player::White);
    }
}

constexpr static inline bool isKingAttacked(const BitBoard& board)
{
    if (board.player == Player::White) {
        return board.whiteKing & gen::getAllAttacks(board, Player::Black);
    } else {
        return board.blackKing & gen::getAllAttacks(board, Player::White);
    }
}

constexpr static inline BitBoard performMove(const BitBoard& board, const movement::Move& move)
{
    BitBoard newBoard = board;

    const auto fromSquare = move.fromSquare();
    const auto toSquare = move.toSquare();

    if (move.isCastleMove()) {
        performCastleMove(newBoard, move);
    } else {
        if (move.isPromotionMove())
            performPromotionMove(newBoard, move);
        else {
            // should be done before moving pieces
            updateCastlingRights(newBoard, fromSquare);
        }

        bitToggleMove(newBoard.whitePawns, fromSquare, toSquare);
        bitToggleMove(newBoard.whiteRooks, fromSquare, toSquare);
        bitToggleMove(newBoard.whiteBishops, fromSquare, toSquare);
        bitToggleMove(newBoard.whiteKnights, fromSquare, toSquare);
        bitToggleMove(newBoard.whiteQueens, fromSquare, toSquare);
        bitToggleMove(newBoard.whiteKing, fromSquare, toSquare);

        bitToggleMove(newBoard.blackPawns, fromSquare, toSquare);
        bitToggleMove(newBoard.blackRooks, fromSquare, toSquare);
        bitToggleMove(newBoard.blackBishops, fromSquare, toSquare);
        bitToggleMove(newBoard.blackKnights, fromSquare, toSquare);
        bitToggleMove(newBoard.blackQueens, fromSquare, toSquare);
        bitToggleMove(newBoard.blackKing, fromSquare, toSquare);
    }

    newBoard.player = nextPlayer(newBoard.player);
    newBoard.roundsCount++;

    return newBoard;
}

constexpr static inline void printBoardDebug(FileLogger& logger, const BitBoard& board)
{
    logger.log("\n******* BitBoard debug: *******\n");

    for (uint8_t row = 8; row > 0; row--) {
        logger << std::format("-{}- ", row);

        for (uint8_t column = 0; column < 8; column++) {
            uint64_t square = 1ULL << (((row - 1) * 8) + column);

            if (square & board.whitePawns) {
                logger << "WP ";
            } else if (square & board.whiteRooks) {
                logger << "WR ";
            } else if (square & board.whiteBishops) {
                logger << "WB ";
            } else if (square & board.whiteKnights) {
                logger << "WH ";
            } else if (square & board.whiteQueens) {
                logger << "WQ ";
            } else if (square & board.whiteKing) {
                logger << "WK ";
            } else if (square & board.blackPawns) {
                logger << "BP ";
            } else if (square & board.blackRooks) {
                logger << "BR ";
            } else if (square & board.blackBishops) {
                logger << "BB ";
            } else if (square & board.blackKnights) {
                logger << "BH ";
            } else if (square & board.blackQueens) {
                logger << "BQ ";
            } else if (square & board.blackKing) {
                logger << "BK ";
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
    logger << "\nRound: " << std::to_string(board.roundsCount);
    logger << "\nCastle: ";
    for (const auto& move : allMoves.getMoves()) {
        if (move.isCastleMove()) {
            logger << move.toString().data() << " ";
        }
    }

    logger << std::format("\n\nMoves[{}]:\n", allMoves.count());
    for (const auto& move : allMoves.getMoves()) {
        logger << std::format("{} [{}]  ", move.toString().data(), evaluation::MoveScoring::score(board, move, 0));
    }

    const auto captures = getAllCapturesSorted(board, 0);
    logger << std::format("\n\nCaptures[{}]:\n", captures.count());
    for (const auto& move : captures.getMoves()) {
        logger << std::format("{} [{}]  ", move.toString().data(), evaluation::MoveScoring::score(board, move, 0));
    }

    logger << "\n\n";
    logger.flush();
}

}
