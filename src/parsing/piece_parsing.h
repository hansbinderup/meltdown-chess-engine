#pragma once

#include "core/board_defs.h"
#include <optional>
#include <string_view>

namespace parsing {

using PiecePlayerPair = std::pair<Piece, Player>;
constexpr static inline std::optional<PiecePlayerPair> piecePlayerFromChar(char c)
{
    switch (c) {
    case 'P':
        return std::make_pair(Pawn, PlayerWhite);
    case 'N':
        return std::make_pair(Knight, PlayerWhite);
    case 'B':
        return std::make_pair(Bishop, PlayerWhite);
    case 'R':
        return std::make_pair(Rook, PlayerWhite);
    case 'Q':
        return std::make_pair(Queen, PlayerWhite);
    case 'K':
        return std::make_pair(King, PlayerWhite);
    case 'p':
        return std::make_pair(Pawn, PlayerBlack);
    case 'n':
        return std::make_pair(Knight, PlayerBlack);
    case 'b':
        return std::make_pair(Bishop, PlayerBlack);
    case 'r':
        return std::make_pair(Rook, PlayerBlack);
    case 'q':
        return std::make_pair(Queen, PlayerBlack);
    case 'k':
        return std::make_pair(King, PlayerBlack);
    };

    return std::nullopt;
}

constexpr static inline char pieceToChar(Piece piece, Player player)
{
    if (player == PlayerWhite) {
        switch (piece) {
        case Pawn:
            return 'P';
        case Knight:
            return 'N';
        case Bishop:
            return 'B';
        case Rook:
            return 'R';
        case Queen:
            return 'Q';
        case King:
            return 'K';
        }
    } else {
        switch (piece) {
        case Pawn:
            return 'p';
        case Knight:
            return 'n';
        case Bishop:
            return 'b';
        case Rook:
            return 'r';
        case Queen:
            return 'q';
        case King:
            return 'k';
        }
    }

    return '?';
}

constexpr static inline std::string_view pieceToUnicode(Piece piece, Player player)
{
    if (player == PlayerWhite) {
        switch (piece) {
        case Pawn:
            return "♙";
        case Knight:
            return "♘";
        case Bishop:
            return "♗";
        case Rook:
            return "♖";
        case Queen:
            return "♕";
        case King:
            return "♔";
        }
    } else {
        switch (piece) {
        case Pawn:
            return "♟";
        case Knight:
            return "♞";
        case Bishop:
            return "♝";
        case Rook:
            return "♜";
        case Queen:
            return "♛";
        case King:
            return "♚";
        }
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
