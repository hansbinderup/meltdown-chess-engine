#pragma once

#include "board_defs.h"
#include <optional>
#include <string_view>

namespace parsing {

constexpr static inline std::optional<Piece> pieceFromChar(char c)
{
    switch (c) {
    case 'P':
        return Piece::WhitePawn;
    case 'N':
        return Piece::WhiteKnight;
    case 'B':
        return Piece::WhiteBishop;
    case 'R':
        return Piece::WhiteRook;
    case 'Q':
        return Piece::WhiteQueen;
    case 'K':
        return Piece::WhiteKing;
    case 'p':
        return Piece::BlackPawn;
    case 'n':
        return Piece::BlackKnight;
    case 'b':
        return Piece::BlackBishop;
    case 'r':
        return Piece::BlackRook;
    case 'q':
        return Piece::BlackQueen;
    case 'k':
        return Piece::BlackKing;
    };

    return std::nullopt;
}

constexpr static inline char pieceToChar(Piece piece)
{
    switch (piece) {
    case WhitePawn:
        return 'P';
    case WhiteKnight:
        return 'N';
    case WhiteBishop:
        return 'B';
    case WhiteRook:
        return 'R';
    case WhiteQueen:
        return 'Q';
    case WhiteKing:
        return 'K';
    case BlackPawn:
        return 'p';
    case BlackKnight:
        return 'n';
    case BlackBishop:
        return 'b';
    case BlackRook:
        return 'r';
    case BlackQueen:
        return 'q';
    case BlackKing:
        return 'k';
    }

    return '?';
}

constexpr static inline std::string_view pieceToUnicode(Piece piece)
{
    switch (piece) {
    case WhitePawn:
        return "♙";
    case WhiteKnight:
        return "♘";
    case WhiteBishop:
        return "♗";
    case WhiteRook:
        return "♖";
    case WhiteQueen:
        return "♕";
    case WhiteKing:
        return "♔";
    case BlackPawn:
        return "♟";
    case BlackKnight:
        return "♞";
    case BlackBishop:
        return "♝";
    case BlackRook:
        return "♜";
    case BlackQueen:
        return "♛";
    case BlackKing:
        return "♚";
    }

    return " ";
}

constexpr static inline std::optional<CastleType> castleFromChar(char c)
{
    switch (c) {
    case 'K':
        return CastleWhiteKingSide;
    case 'Q':
        return CastleWhiteQueenSide;
    case 'k':
        return CastleBlackKingSide;
    case 'q':
        return CastleBlackQueenSide;
    case '-':
        return CastleNone;
    }

    return std::nullopt;
}

}
