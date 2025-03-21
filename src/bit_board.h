#pragma once

#include "magic_enum/magic_enum.hpp"
#include <array>
#include <board_defs.h>
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
    }

    constexpr void updateOccupation()
    {
        occupation[White] = pieces[WhitePawn] | pieces[WhiteRook] | pieces[WhiteBishop] | pieces[WhiteKnight] | pieces[WhiteQueen] | pieces[WhiteKing];
        occupation[Black] = pieces[BlackPawn] | pieces[BlackRook] | pieces[BlackBishop] | pieces[BlackKnight] | pieces[BlackQueen] | pieces[BlackKing];
        occupation[Both] = occupation[White] | occupation[Black];
    }

    constexpr std::optional<Piece> getPieceAtSquare(uint64_t square) const
    {
        for (const auto piece : magic_enum::enum_values<Piece>()) {
            if (square & pieces[piece]) {
                return piece;
            }
        }

        return std::nullopt;
    }

    constexpr bool isQuietPosition() const
    {
        return (attacks[PlayerWhite] & occupation[PlayerBlack]) == 0 && (attacks[PlayerBlack] & occupation[PlayerWhite]) == 0;
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
};

