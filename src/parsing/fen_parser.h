#pragma once

#include <iostream>
#include <src/parsing/input_parsing.h>

namespace parsing {

class FenParser {
    enum Parts : uint8_t {
        PiecePlacement = 0,
        SideToMove,
        CastlingAbility,
        EnPassantTargetSquare,
        HalfMoveClock,
        FullMoveCounter,
    };

public:
    FenParser(const FenParser&) = delete;
    FenParser(FenParser&&) = delete;
    FenParser& operator=(const FenParser&) = delete;
    FenParser& operator=(FenParser&&) = delete;

    static inline std::optional<BitBoard> parse(std::string_view sv)
    {
        bool success { true };
        BitBoard board;

        for (const auto part : magic_enum::enum_values<Parts>()) {
            if (!success)
                break;

            switch (part) {
            case Parts::PiecePlacement:
                success &= parsePiecePlacement(sv, board);
                break;
            case Parts::SideToMove:
                success &= parseSideToMove(sv, board);
                break;
            case Parts::CastlingAbility:
                success &= parseCastlingAbility(sv, board);
                break;
            case Parts::EnPassantTargetSquare:
                success &= parseEnPassantTargetSquare(sv, board);
                break;
            case Parts::HalfMoveClock:
                success &= parseHalfMoveClock(sv, board);
                break;
            case Parts::FullMoveCounter:
                success &= parseFullMoveCounter(sv, board);
                break;
            }
        }

        if (success)
            return board;

        std::cerr << "invalid fen string" << std::endl;
        return std::nullopt;
    }

    static inline bool parsePiecePlacement(std::string_view& sv, BitBoard& board)
    {
        const auto input = sv_next_split(sv);
        if (!input.has_value())
            return false;

        uint8_t row = 7;
        uint8_t column = 0;

        for (const auto c : input.value()) {
            if (c == '/') {
                row--;
                column = 0;
            } else {
                if (row > 7 || column > 7) {
                    return false;
                }

                const auto piece = pieceFromChar(c);
                if (piece.has_value()) {
                    const uint8_t square = (row * 8) + column;
                    board.pieces[piece.value()] |= 1ULL << square;
                    column++;
                } else {
                    const uint8_t skip = c - '0';
                    column += skip;
                }
            }
        }

        /* some pieces are missing - invalid fen string */
        if (!(row == 0 && column == 8)) {
            return false;
        }

        board.updateOccupation();

        return true;
    }

    static inline bool parseSideToMove(std::string_view& sv, BitBoard& board)
    {
        const auto input = sv_next_split(sv);
        if (!input.has_value())
            return false;

        for (const auto c : input.value()) {
            if (c == 'w') {
                board.player = Player::White;
                return true;
            } else if (c == 'b') {
                board.player = Player::Black;
                return true;
            }
        }

        return false;
    }

    static inline bool parseCastlingAbility(std::string_view& sv, BitBoard& board)
    {
        const auto input = sv_next_split(sv);
        if (!input.has_value())
            return false;

        for (const auto c : input.value()) {
            const auto castle = castleFromChar(c);
            if (castle.has_value()) {
                switch (castle.value()) {
                case CastleWhiteKingSide:
                    board.whiteCastlingRights |= s_whiteKingSideCastleMask;
                    break;
                case CastleWhiteQueenSide:
                    board.whiteCastlingRights |= s_whiteQueenSideCastleMask;
                    break;
                case CastleBlackKingSide:
                    board.blackCastlingRights |= s_blackKingSideCastleMask;
                    break;
                case CastleBlackQueenSide:
                    board.blackCastlingRights |= s_blackQueenSideCastleMask;
                    break;
                case CastleNone:
                    board.whiteCastlingRights = 0;
                    board.blackCastlingRights = 0;
                }
            }
        }

        return true;
    }

    static inline bool parseEnPassantTargetSquare(std::string_view& sv, BitBoard& board)
    {
        const auto input = sv_next_split(sv);
        if (!input.has_value())
            return false;

        if (input.value().size() == 0)
            return false;

        if (input.value().at(0) == '-') {
            board.enPessant.reset();
            return true;
        }

        if (input.value().size() < 2)
            return false;

        const uint8_t column = input.value().at(0) - 'a';
        const uint8_t row = input.value().at(1) - '1';
        const uint8_t square = (row * 8) + column;

        std::cout << input.value() << " " << std::to_string(row) << " " << std::to_string(column) << " " << std::to_string(square) << std::endl;

        if (square > 63) {
            return false;
        }

        board.enPessant = 1ULL << square;
        return true;
    }

    static inline bool parseHalfMoveClock(std::string_view& sv, BitBoard& board)
    {
        const auto input = sv_next_split(sv);
        if (!input.has_value())
            return false;

        if (const auto moveCounter = parsing::to_number(input.value())) {
            board.halfMoves = moveCounter.value();
            return true;
        }

        return false;
    }

    static inline bool parseFullMoveCounter(std::string_view& sv, BitBoard& board)
    {
        if (const auto moveCounter = parsing::to_number(sv)) {
            board.fullMoves = moveCounter.value();
            return true;
        }

        return false;
    }

private:
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
};
}
