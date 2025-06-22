#pragma once

#include "core/board_defs.h"
#include "magic_enum/magic_enum.hpp"
#include <array>
#include <cstdint>
#include <optional>

enum Occupation {
    White,
    Black,
    Both,
};

struct BitBoard {
    constexpr void updateOccupation()
    {
        occupation[White] = pieces[WhitePawn] | pieces[WhiteRook] | pieces[WhiteBishop] | pieces[WhiteKnight] | pieces[WhiteQueen] | pieces[WhiteKing];
        occupation[Black] = pieces[BlackPawn] | pieces[BlackRook] | pieces[BlackBishop] | pieces[BlackKnight] | pieces[BlackQueen] | pieces[BlackKing];
        occupation[Both] = occupation[White] | occupation[Black];
    }

    template<Player player>
    constexpr std::optional<Piece> getAttackerAtSquare(uint64_t square) const
    {
        if constexpr (player == PlayerWhite) {
            for (const auto piece : s_whitePieces) {
                if (square & pieces[piece]) {
                    return piece;
                }
            }
        } else {
            for (const auto piece : s_blackPieces) {
                if (square & pieces[piece]) {
                    return piece;
                }
            }
        }

        return std::nullopt;
    }

    // Helper: calling within loops will mean redundant colour checks
    constexpr std::optional<Piece> getAttackerAtSquare(uint64_t square, Player player) const
    {
        if (player == PlayerWhite)
            return getAttackerAtSquare<PlayerWhite>(square);
        else
            return getAttackerAtSquare<PlayerBlack>(square);
    }

    template<Player player>
    constexpr std::optional<Piece> getTargetAtSquare(uint64_t square) const
    {
        if constexpr (player == PlayerWhite) {
            for (const auto piece : s_blackPieces) {
                if (square & pieces[piece]) {
                    return piece;
                }
            }
        } else {
            for (const auto piece : s_whitePieces) {
                if (square & pieces[piece]) {
                    return piece;
                }
            }
        }

        return std::nullopt;
    }

    // Helper: calling within loops will mean redundant colour checks
    constexpr std::optional<Piece> getTargetAtSquare(uint64_t square, Player player) const
    {
        if (player == PlayerWhite)
            return getTargetAtSquare<PlayerWhite>(square);
        else
            return getTargetAtSquare<PlayerBlack>(square);
    }

    constexpr bool isQuietPosition() const
    {
        return (attacks[PlayerWhite] & occupation[PlayerBlack]) == 0 && (attacks[PlayerBlack] & occupation[PlayerWhite]) == 0;
    }

    /* this is a very primitive way of checking for zugzwang position
     * zugzwang is _very rare_ to happen outside of these conditions
     * so for most cases this should be _super good enough_ */
    constexpr inline bool hasZugzwangProneMaterial() const
    {
        if (player == PlayerWhite)
            return occupation[PlayerWhite] == (pieces[WhitePawn] | pieces[WhiteKing]);
        else
            return occupation[PlayerBlack] == (pieces[BlackPawn] | pieces[BlackKing]);
    }

    inline bool hasInsufficientMaterial() const
    {
        /* cheapest check first - always sufficient material if more than 4 pieces */
        const int totalPieces = std::popcount(occupation[Both]);
        if (totalPieces > 4) {
            return false;
        }

        /* if pawns or majors are on the board, there is always sufficient material */
        const uint64_t majorsOrPawns = pieces[WhitePawn] | pieces[BlackPawn] | pieces[WhiteRook] | pieces[BlackRook] | pieces[WhiteQueen] | pieces[BlackQueen];
        if (std::popcount(majorsOrPawns) != 0) {
            return false;
        }

        if (totalPieces <= 2) {
            /* only kings on the board */
            return true;
        }

        if (totalPieces == 3) {
            const uint64_t minors = pieces[WhiteBishop] | pieces[BlackBishop] | pieces[WhiteKnight] | pieces[BlackKnight];

            /* one minor piece + kings is still a draw */
            return std::popcount(minors) == 1;
        }

        if (totalPieces == 4) {
            const uint64_t bishops = pieces[WhiteBishop] | pieces[BlackBishop];

            /* check for exactly one bishop per side */
            const bool oneWhiteBishop = std::popcount(pieces[WhiteBishop]) == 1;

            /* check if bishops are on opposite color squares */
            const bool bishopOnLight = std::popcount(s_lightSquares & bishops) == 1;
            const bool bishopOnDark = std::popcount(s_darkSquares & bishops) == 1;

            return oneWhiteBishop && bishopOnLight && bishopOnDark;
        }

        /* all other cases have potential for sufficient material */
        return false;
    }

    std::array<uint64_t, magic_enum::enum_count<Piece>()> pieces {};
    std::array<uint64_t, magic_enum::enum_count<Occupation>()> occupation {};
    std::array<uint64_t, magic_enum::enum_count<Player>()> attacks {};

    // castling
    uint64_t castlingRights {};

    // which player to perform next move
    Player player;

    std::optional<BoardPosition> enPessant;

    // amount of rounds that the game has been played
    uint16_t fullMoves {};
    uint16_t halfMoves {};

    /* hashes for the current position */
    uint64_t hash {};
    uint64_t kpHash {};
};
