#pragma once

#include <cstdint>
#include <optional>
#include <src/board_defs.h>

struct BitBoard {
    constexpr void reset()
    {
        // Binary represensation of each piece at start of game
        whitePawns = { 0xffULL << s_secondRow };
        whiteRooks = { 0x81ULL };
        whiteBishops = { 0x24ULL };
        whiteKnights = { 0x42ULL };
        whiteQueens = { 0x08ULL };
        whiteKing = { 0x10ULL };

        blackPawns = { 0xffULL << s_seventhRow };
        blackRooks = { 0x81ULL << s_eightRow };
        blackBishops = { 0x24ULL << s_eightRow };
        blackKnights = { 0x42ULL << s_eightRow };
        blackQueens = { 0x08ULL << s_eightRow };
        blackKing = { 0x10ULL << s_eightRow };

        whiteCastlingRights = { s_whiteQueenSideCastleMask | s_whiteKingSideCastleMask };
        blackCastlingRights = { s_blackQueenSideCastleMask | s_blackKingSideCastleMask };

        player = Player::White;
        roundsCount = 0;
    }

    constexpr uint64_t getWhiteOccupation() const
    {
        return whitePawns | whiteRooks | whiteBishops | whiteKnights | whiteQueens | whiteKing;
    }

    constexpr uint64_t getBlackOccupation() const
    {
        return blackPawns | blackRooks | blackBishops | blackKnights | blackQueens | blackKing;
    }

    constexpr uint64_t getAllOccupation() const
    {
        return getWhiteOccupation() | getBlackOccupation();
    }

    constexpr uint16_t getRoundsCount() const
    {
        return roundsCount;
    }

    constexpr std::optional<Piece> getPieceAtSquare(uint64_t square) const
    {
        if (square & whitePawns) {
            return Piece::WhitePawn;
        } else if (square & whiteKnights) {
            return Piece::WhiteKnight;
        } else if (square & whiteBishops) {
            return Piece::WhiteBishop;
        } else if (square & whiteRooks) {
            return Piece::WhiteRook;
        } else if (square & whiteQueens) {
            return Piece::WhiteQueen;
        } else if (square & whiteKing) {
            return Piece::WhiteKing;
        } else if (square & blackPawns) {
            return Piece::BlackPawn;
        } else if (square & blackKnights) {
            return Piece::BlackKnight;
        } else if (square & blackBishops) {
            return Piece::BlackBishop;
        } else if (square & blackRooks) {
            return Piece::BlackRook;
        } else if (square & blackQueens) {
            return Piece::BlackQueen;
        } else if (square & blackKing) {
            return Piece::BlackKing;
        }

        return std::nullopt;
    }

    // Bitboard represensation of each pieces
    uint64_t whitePawns;
    uint64_t whiteRooks;
    uint64_t whiteBishops;
    uint64_t whiteKnights;
    uint64_t whiteQueens;
    uint64_t whiteKing;

    uint64_t blackPawns;
    uint64_t blackRooks;
    uint64_t blackBishops;
    uint64_t blackKnights;
    uint64_t blackQueens;
    uint64_t blackKing;

    // castling
    uint64_t whiteCastlingRights;
    uint64_t blackCastlingRights;

    // which player to perform next move
    Player player;

    // amount of rounds that the game has been played
    uint16_t roundsCount;
};

