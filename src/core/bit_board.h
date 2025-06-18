#pragma once

#include "core/board_defs.h"
#include "core/zobrist_hashing.h"
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
    constexpr void reset()
    {
        // Binary represensation of each piece at start of game
        pieces[Piece::WhitePawn] = { 0xffULL << s_secondRow };
        pieces[Piece::WhiteRook] = { 0x81ULL };
        pieces[Piece::WhiteBishop] = { 0x24ULL };
        pieces[Piece::WhiteKnight] = { 0x42ULL };
        pieces[Piece::WhiteQueen] = { 0x08ULL };
        pieces[Piece::WhiteKing] = { 0x10ULL };

        pieces[Piece::BlackPawn] = { 0xffULL << s_seventhRow };
        pieces[Piece::BlackRook] = { 0x81ULL << s_eightRow };
        pieces[Piece::BlackBishop] = { 0x24ULL << s_eightRow };
        pieces[Piece::BlackKnight] = { 0x42ULL << s_eightRow };
        pieces[Piece::BlackQueen] = { 0x08ULL << s_eightRow };
        pieces[Piece::BlackKing] = { 0x10ULL << s_eightRow };

        castlingRights = { CastleWhiteKingSide | CastleWhiteQueenSide | CastleBlackKingSide | CastleBlackQueenSide };

        updateOccupation();

        player = PlayerWhite;
        fullMoves = 0;
        halfMoves = 0;
        enPessant.reset();

        /* should be done last to reflect the full board state! */
        updateHash();
    }

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

    void updateHash()
    {
        hash = 0;
        for (const auto piece : magic_enum::enum_values<Piece>()) {
            uint64_t pieceCopy = pieces[piece];

            utils::bitIterate(pieceCopy, [&](BoardPosition pos) {
                core::hashPiece(piece, pos, this->hash);
            });
        }

        core::hashCastling(castlingRights, this->hash);

        if (enPessant.has_value()) {
            core::hashEnpessant(enPessant.value(), this->hash);
        }

        /* only need to hash black as we only have two players */
        if (player == PlayerBlack) {
            core::hashPlayer(this->hash);
        }
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
    uint32_t fullMoves {};
    uint32_t halfMoves {};

    /* hash for the current position */
    uint64_t hash {};
};
