#pragma once

#include "parsing/input_parsing.h"
#include "parsing/piece_parsing.h"

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

            auto input = sv_next_split(sv);
            if (!input.has_value()) {
                /* if there's no more words left or the next one is "moves" it means
                 * that we've received a partial FEN string.
                 * This can be alright, if we've already parsed PiecePlacement and SideToMove */
                if (sv == "moves" || sv.empty()) {
                    success &= part < SideToMove;
                    break;
                }

                /* there was no space found - the string might still not be empty though, so check for that first! */
                input = sv;
            }

            switch (part) {
            case Parts::PiecePlacement:
                success &= parsePiecePlacement(*input, board);
                break;
            case Parts::SideToMove:
                success &= parseSideToMove(*input, board);
                break;
            case Parts::CastlingAbility:
                success &= parseCastlingAbility(*input, board);
                break;
            case Parts::EnPassantTargetSquare:
                success &= parseEnPassantTargetSquare(*input, board);
                break;
            case Parts::HalfMoveClock:
                success &= parseHalfMoveClock(*input, board);
                break;
            case Parts::FullMoveCounter:
                success &= parseFullMoveCounter(*input, board);
                break;
            }

            /* make sure to break out if we're dealing with the last piece of data */
            if (*input == sv) {
                break;
            }
        }

        /* last thing to do - update hashes so they reflect the full board state */
        board.hash = core::generateHash(board);
        board.kpHash = core::generateKingPawnHash(board);

        if (success)
            return board;

        fmt::println("Invalid FEN string");

        return std::nullopt;
    }

    static inline bool parsePiecePlacement(std::string_view input, BitBoard& board)
    {
        uint8_t row = 7;
        uint8_t column = 0;

        for (const auto c : input) {
            if (c == '/') {
                row--;
                column = 0;
            } else {
                if (row > 7 || column > 7) {
                    return false;
                }

                const auto piecePlayer = parsing::piecePlayerFromChar(c);
                if (piecePlayer.has_value()) {
                    const auto [piece, player] = piecePlayer.value();
                    const auto pos = intToBoardPosition((row * 8) + column);
                    const auto square = utils::positionToSquare(pos);
                    board.pieces[piece] |= square;
                    board.occupation[player] |= square;
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

        board.occupation[Both] = board.occupation[PlayerWhite] | board.occupation[PlayerBlack];
        board.attacks[PlayerWhite] = attackgen::getAllAttacks<PlayerWhite>(board);
        board.attacks[PlayerBlack] = attackgen::getAllAttacks<PlayerBlack>(board);

        return true;
    }

    static inline bool parseSideToMove(std::string_view input, BitBoard& board)
    {
        for (const auto c : input) {
            if (c == 'w') {
                board.player = PlayerWhite;
                return true;
            } else if (c == 'b') {
                board.player = PlayerBlack;
                return true;
            }
        }

        return false;
    }

    static inline bool parseCastlingAbility(std::string_view input, BitBoard& board)
    {
        for (const auto c : input) {
            const auto castle = parsing::castleFromChar(c);
            if (castle.has_value()) {
                board.castlingRights |= castle.value();
            }
        }

        return true;
    }

    static inline bool parseEnPassantTargetSquare(std::string_view input, BitBoard& board)
    {
        if (input.size() == 0)
            return false;

        if (input.at(0) == '-') {
            board.enPessant.reset();
            return true;
        }

        if (input.size() < 2)
            return false;

        const uint8_t column = input.at(0) - 'a';
        const uint8_t row = input.at(1) - '1';
        const auto pos = intToBoardPosition((row * 8) + column);

        if (pos > 63) {
            return false;
        }

        board.enPessant = pos;
        return true;
    }

    static inline bool parseHalfMoveClock(std::string_view input, BitBoard& board)
    {
        if (const auto moveCounter = parsing::to_number(input)) {
            board.halfMoves = moveCounter.value();
            return true;
        }

        return false;
    }

    static inline bool parseFullMoveCounter(std::string_view input, BitBoard& board)
    {
        /* last entry - there might still be data in the string */
        if (const auto moveCounter = parsing::to_number(input)) {
            board.fullMoves = moveCounter.value();
            return true;
        }

        return false;
    }
};

}
