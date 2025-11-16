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
    template<Player player>
    constexpr std::optional<Piece> getAttackerAtSquare(uint64_t square) const
    {
        for (const auto piece : magic_enum::enum_values<Piece>()) {
            if (square & pieces[piece] & occupation[player]) {
                return piece;
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
        constexpr auto opponent = nextPlayer(player);
        return getAttackerAtSquare<opponent>(square);
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
            return occupation[PlayerWhite] == ((pieces[Pawn] | pieces[King]) & occupation[PlayerWhite]);
        else
            return occupation[PlayerBlack] == ((pieces[Pawn] | pieces[King]) & occupation[PlayerBlack]);
    }

    inline bool hasInsufficientMaterial() const
    {
        /* cheapest check first - always sufficient material if more than 4 pieces */
        const int totalPieces = std::popcount(occupation[Both]);
        if (totalPieces > 4) {
            return false;
        }

        /* if pawns or majors are on the board, there is always sufficient material */
        const uint64_t majorsOrPawns = pieces[Pawn] | pieces[Rook] | pieces[Queen];
        if (std::popcount(majorsOrPawns) != 0) {
            return false;
        }

        if (totalPieces <= 2) {
            /* only kings on the board */
            return true;
        }

        if (totalPieces == 3) {
            const uint64_t minors = pieces[Bishop] | pieces[Knight];

            /* one minor piece + kings is still a draw */
            return std::popcount(minors) == 1;
        }

        if (totalPieces == 4) {
            const uint64_t bishops = pieces[Bishop];

            /* check for exactly one bishop per side */
            const bool oneWhiteBishop = std::popcount(pieces[Bishop] & occupation[PlayerWhite]) == 1;

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
