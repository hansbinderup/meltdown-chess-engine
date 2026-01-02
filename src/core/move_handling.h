#pragma once

#include "core/attack_generation.h"
#include "core/bit_board.h"
#include "core/mask_tables.h"
#include "core/transposition.h"
#include "core/zobrist_hashing.h"
#include "movegen/move_generation.h"
#include "movegen/move_types.h"
#include "parsing/piece_parsing.h"
#include "utils/formatters.h"

#include "fmt/ranges.h"
#include "magic_enum/magic_enum.hpp"

namespace core {

namespace {

template<Player player>
static inline void updateHashes(BitBoard& newBoard, BoardPosition pos, Piece type)
{
    core::hashPiece(type, pos, newBoard.hash);

    if (utils::isPawn<player>(type)) {
        core::hashPiece(type, pos, newBoard.pawnHash);
        core::hashPiece(type, pos, newBoard.kpHash);
    } else {
        core::hashPiece(type, pos, newBoard.nonPawnHashes[player]);
    }

    if (utils::isMinor<player>(type)) {
        core::hashPiece(type, pos, newBoard.minorHash);
    } else if (utils::isMajor<player>(type)) {
        core::hashPiece(type, pos, newBoard.majorHash);
    } else if (utils::isKing<player>(type)) {
        core::hashPiece(type, pos, newBoard.kpHash);
    }
}

template<Player player>
constexpr static inline void clearPiece(uint64_t& piece, BoardPosition pos, Piece type, BitBoard& newBoard)
{
    piece &= ~utils::positionToSquare(pos);
    updateHashes<player>(newBoard, pos, type);
}

template<Player player>
constexpr static inline void setPiece(uint64_t& piece, BoardPosition pos, Piece type, BitBoard& newBoard)
{
    piece |= utils::positionToSquare(pos);
    updateHashes<player>(newBoard, pos, type);
}

template<Player player>
constexpr static inline void movePiece(uint64_t& piece, BoardPosition fromPos, BoardPosition toPos, Piece type, BitBoard& newBoard)
{
    clearPiece<player>(piece, fromPos, type, newBoard);
    setPiece<player>(piece, toPos, type, newBoard);
}

constexpr inline void updateCastlingRights(BitBoard& board, BoardPosition pos)
{
    board.castlingRights &= ~s_castlingRightMaskTable[pos];
}

template<Player player>
constexpr void performCastleMove(BitBoard& newBoard, movegen::Move move)
{
    const BoardPosition fromPos = move.fromPos();
    const BoardPosition toPos = move.toPos();

    switch (move.castleType<player>()) {
    case CastleWhiteKingSide: {
        movePiece<player>(newBoard.pieces[WhiteKing], fromPos, toPos, WhiteKing, newBoard);
        movePiece<player>(newBoard.pieces[WhiteRook], H1, F1, WhiteRook, newBoard);
    } break;

    case CastleWhiteQueenSide: {
        movePiece<player>(newBoard.pieces[WhiteKing], fromPos, toPos, WhiteKing, newBoard);
        movePiece<player>(newBoard.pieces[WhiteRook], A1, D1, WhiteRook, newBoard);
    } break;
    case CastleBlackKingSide: {
        movePiece<player>(newBoard.pieces[BlackKing], fromPos, toPos, BlackKing, newBoard);
        movePiece<player>(newBoard.pieces[BlackRook], H8, F8, BlackRook, newBoard);
    } break;
    case CastleBlackQueenSide: {
        movePiece<player>(newBoard.pieces[BlackKing], fromPos, toPos, BlackKing, newBoard);
        movePiece<player>(newBoard.pieces[BlackRook], A8, D8, BlackRook, newBoard);
    } break;
    case CastleNone:
        assert(false);
        break;
    }
}

template<Player player>
constexpr void performPromotionMove(BitBoard& newBoard, movegen::Move move)
{
    constexpr bool isWhite = player == PlayerWhite;
    constexpr auto type = isWhite ? WhitePawn : BlackPawn;
    constexpr Player opponent = nextPlayer(player);

    // first clear to be promoted pawn
    uint64_t& pawns = newBoard.pieces[type];
    clearPiece<player>(pawns, move.fromPos(), type, newBoard);

    /* clear piece that will be taken if capture */
    if (move.isCapture()) {
        if (const auto victim = newBoard.getTargetAtSquare<player>(move.toSquare())) {
            clearPiece<opponent>(newBoard.pieces[victim.value()], move.toPos(), victim.value(), newBoard);
        }
    }

    switch (move.promotionType()) {
    case PromotionNone:
        assert(false);
        return;
    case PromotionQueen: {
        constexpr auto type = isWhite ? WhiteQueen : BlackQueen;
        setPiece<player>(newBoard.pieces[type], move.toPos(), type, newBoard);
    } break;
    case PromotionKnight: {
        constexpr auto type = isWhite ? WhiteKnight : BlackKnight;
        setPiece<player>(newBoard.pieces[type], move.toPos(), type, newBoard);
    } break;
    case PromotionBishop: {
        constexpr auto type = isWhite ? WhiteBishop : BlackBishop;
        setPiece<player>(newBoard.pieces[type], move.toPos(), type, newBoard);
    } break;
    case PromotionRook: {
        constexpr auto type = isWhite ? WhiteRook : BlackRook;
        setPiece<player>(newBoard.pieces[type], move.toPos(), type, newBoard);
    } break;
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
constexpr static inline uint64_t enpessantCaptureSquare(uint64_t square)
{
    if constexpr (player == PlayerWhite) {
        return square >> 8;
    } else {
        return square << 8;
    }
}

constexpr static inline uint64_t enpessantCaptureSquare(uint64_t square, Player player)
{
    if (player == PlayerWhite) {
        return enpessantCaptureSquare<PlayerWhite>(square);
    } else {
        return enpessantCaptureSquare<PlayerBlack>(square);
    }
}

template<Player player>
constexpr void performEnpessantMove(BitBoard& newBoard, movegen::Move move)
{
    constexpr Piece ourPawn = player == PlayerWhite ? WhitePawn : BlackPawn;
    constexpr Piece theirPawn = player == PlayerWhite ? BlackPawn : WhitePawn;
    constexpr Player opponent = nextPlayer(player);

    const auto fromPos = move.fromPos();
    const auto toPos = move.toPos();
    const auto capturePos = enpessantCapturePosition<player>(toPos);

    movePiece<player>(newBoard.pieces[ourPawn], fromPos, toPos, ourPawn, newBoard);
    clearPiece<opponent>(newBoard.pieces[theirPawn], capturePos, theirPawn, newBoard);
}

template<Player player>
constexpr BitBoard performMove(const BitBoard& board, movegen::Move move)
{
    constexpr Player opponent = nextPlayer(player);

    BitBoard newBoard = board;

    const auto fromPos = move.fromPos();
    const auto toPos = move.toPos();
    const auto pieceType = board.getAttackerAtSquare<player>(move.fromSquare()).value();

    if (move.isCastleMove()) {
        performCastleMove<player>(newBoard, move);
    } else if (move.takeEnPessant()) {
        performEnpessantMove<player>(newBoard, move);
    } else if (move.isPromotionMove()) {
        performPromotionMove<player>(newBoard, move);
    } else {
        if (move.isCapture()) {
            if (const auto victim = board.getTargetAtSquare<player>(move.toSquare())) {
                clearPiece<opponent>(newBoard.pieces[victim.value()], move.toPos(), victim.value(), newBoard);
            }
        }

        movePiece<player>(newBoard.pieces[pieceType], fromPos, toPos, pieceType, newBoard);
    }

    hashCastling(newBoard.castlingRights, newBoard.hash); // remove current hash
    updateCastlingRights(newBoard, fromPos);
    updateCastlingRights(newBoard, toPos);
    hashCastling(newBoard.castlingRights, newBoard.hash); // add new hash

    if (board.enPessant.has_value()) {
        newBoard.enPessant.reset();
        hashEnpessant(board.enPessant.value(), newBoard.hash);
    }

    if (move.isDoublePush()) {
        newBoard.enPessant = enpessantCapturePosition<player>(toPos);
        hashEnpessant(newBoard.enPessant.value(), newBoard.hash);
    }

    newBoard.updateOccupation();

    newBoard.attacks[PlayerWhite] = attackgen::getAllAttacks<PlayerWhite>(newBoard);
    newBoard.attacks[PlayerBlack] = attackgen::getAllAttacks<PlayerBlack>(newBoard);

    /* player making the move is black -> inc full moves */
    if constexpr (player == PlayerBlack)
        newBoard.fullMoves++;

    if (move.isCapture() || utils::isPawn<player>(pieceType))
        newBoard.halfMoves = 0;
    else
        newBoard.halfMoves++;

    newBoard.player = nextPlayer(player);
    hashPlayer(newBoard.hash);

    /* prefetch as soon as we have calculated the key/hash */
    core::TranspositionTable::prefetch(newBoard.hash);

    return newBoard;
}

// Helper: using inside loops means redundant colour checks
constexpr BitBoard performMove(const BitBoard& board, movegen::Move move)
{
    if (board.player == PlayerWhite)
        return performMove<PlayerWhite>(board, move);
    else
        return performMove<PlayerBlack>(board, move);
}

constexpr void printPositionDebug(const BitBoard& board)
{
    fmt::println("");

    for (uint8_t row = 8; row > 0; row--) {
        fmt::print("{} ", row);

        for (uint8_t column = 0; column < 8; column++) {
            const auto pos = intToBoardPosition((row - 1) * 8 + column);
            uint64_t square = utils::positionToSquare(pos);
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
        board.hash);

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
