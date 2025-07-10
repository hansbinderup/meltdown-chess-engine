#pragma once

#include "core/bit_board.h"
#include "core/board_defs.h"

#include "core/move_handling.h"
#include "movegen/bishops.h"
#include "movegen/kings.h"
#include "movegen/knights.h"
#include "movegen/move_types.h"
#include "movegen/rooks.h"

#include <cstdint>

namespace evaluation {

namespace {

template<Player player>
constexpr auto generatePawnTable()
{
    /* pawn table with positions where pawns will be when attacking given target
     * ie. opposite of attack generation */
    std::array<uint64_t, s_amountSquares> table {};

    if constexpr (player == PlayerWhite) {
        for (uint8_t pos = A3; pos <= H8; pos++) {
            const auto file = pos % 8;

            if (file > 0) {
                table[pos] |= 1ULL << (pos - 9);
            }

            if (file < 7) {
                table[pos] |= 1ULL << (pos - 7);
            }
        }
    } else {
        for (uint8_t pos = A1; pos <= H6; pos++) {
            const auto file = pos % 8;

            if (file > 0) {
                table[pos] |= 1ULL << (pos + 7);
            }

            if (file < 7) {
                table[pos] |= 1ULL << (pos + 9);
            }
        }
    }

    return table;
}

}

class SeeSwap {
public:
    static inline int32_t run(const BitBoard& board, movegen::Move move)
    {
        /* this only evaluates the see swap score for captures - used for move ordering */
        assert(move.isCapture());

        const uint64_t fromSquare = move.fromSquare();
        const uint64_t toSquare = move.toSquare();

        /* remove our current move's piece - it's "assumed" to already have been moved to the target square */
        uint64_t occ = board.occupation[Both] & ~fromSquare;

        /* the position we are operation on - all attacks will be targeted here */
        const BoardPosition target = move.toPos();

        int depth = 0;
        std::array<int32_t, 32> gain {}; // Stores gains for each exchange depth

        /* piece that will track the scoring of next piece */
        Piece nextPiece = move.isPromotionMove()
            ? static_cast<Piece>(promotionToColorlessPiece(move.promotionType()))
            : board.getAttackerAtSquare(fromSquare, board.player).value();

        Player player = board.player;

        /* we need to clear en-pessant manually as the capture square is different from the target square */
        if (move.takeEnPessant()) {
            gain[depth] = s_pieceValues[Pawn];
            occ &= ~core::enpessantCaptureSquare(toSquare, player);
        } else {
            const auto initialPiece = board.getTargetAtSquare(toSquare, player).value();
            gain[depth] = s_pieceValues[initialPiece];
        }

        /* initial attack mask based on our "new board occupation" */
        uint64_t attackers = getAttackers(board, target, occ);

        while (attackers) {
            depth++;

            player = nextPlayer(player);
            const auto piece = getLeastValuableAttacker(board, attackers, occ, player);
            if (!piece.has_value())
                break; // No more attackers

            /* update speculative score */
            gain[depth] = s_pieceValues[nextPiece] - gain[depth - 1];
            nextPiece = *piece;

            /* update attackers after removing captured piece */
            attackers = getAttackers(board, target, occ);
        }

        /* Backtrack the sequence and evaluate the score */
        while (--depth > 0) {
            gain[depth - 1] = -std::max(-gain[depth - 1], gain[depth]);
        }

        return gain[0];
    }

private:
    constexpr static inline uint64_t getAttackers(const BitBoard& board, BoardPosition target, uint64_t occ)
    {
        return s_whitePawnTable[target] | s_blackPawnTable[target]
            | (movegen::getKnightMoves(target) & (board.pieces[WhiteKnight] | board.pieces[BlackKnight]))
            | (movegen::getBishopMoves(target, occ) & (board.pieces[WhiteBishop] | board.pieces[WhiteQueen] | board.pieces[BlackBishop] | board.pieces[BlackQueen]))
            | (movegen::getRookMoves(target, occ) & (board.pieces[WhiteRook] | board.pieces[WhiteQueen] | board.pieces[BlackRook] | board.pieces[BlackQueen]))
            | (movegen::getKingMoves(target) & (board.pieces[WhiteKing] | board.pieces[BlackKing]));
    }

    constexpr static inline std::optional<Piece> getLeastValuableAttacker(const BitBoard& board, uint64_t attackers, uint64_t& occ, Player player)
    {
        if (player == PlayerWhite) {
            for (const auto piece : s_whitePieces) {
                uint64_t subset = attackers & occ & board.pieces[piece];
                if (subset) {
                    occ &= ~subset; /* clear the piece we just found */
                    return piece;
                }
            }
        } else {
            for (const auto piece : s_blackPieces) {
                uint64_t subset = attackers & occ & board.pieces[piece];
                if (subset) {
                    occ &= ~subset; /* clear the piece we just found */
                    return piece;
                }
            }
        }

        return std::nullopt; /* attackers must be empty */
    }

    constexpr static inline auto s_whitePawnTable = generatePawnTable<PlayerWhite>();
    constexpr static inline auto s_blackPawnTable = generatePawnTable<PlayerBlack>();
};
}
