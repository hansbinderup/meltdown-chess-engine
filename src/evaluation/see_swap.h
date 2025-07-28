#pragma once

#include "core/bit_board.h"
#include "core/board_defs.h"

#include "core/move_handling.h"
#include "movegen/bishops.h"
#include "movegen/kings.h"
#include "movegen/knights.h"
#include "movegen/move_types.h"
#include "movegen/rooks.h"
#include "spsa/parameters.h"

#include <cstdint>

namespace evaluation {

class SeeSwap {
public:
    static inline int32_t run(const BitBoard& board, movegen::Move move)
    {
        /* currently only works for captures */
        assert(move.isCapture());

        Player player = board.player;
        const uint64_t fromSquare = move.fromSquare();
        const uint64_t toSquare = move.toSquare();

        /* the position we are operation on - all attacks will be targeted here */
        const BoardPosition target = move.toPos();

        int depth = 0;
        std::array<int32_t, 32> gain {}; // Stores gains for each exchange depth

        const auto promotionPiece = promotionToColorlessPiece(move.promotionType());

        /* piece that will track the scoring of next piece */
        Piece nextPiece = promotionPiece.has_value()
            ? static_cast<Piece>(promotionPiece.value())
            : board.getAttackerAtSquare(fromSquare, board.player).value();

        /* remove our current move's piece - it's "assumed" to already have been moved to the target square */
        uint64_t occ = board.occupation[Both] & ~fromSquare;

        /* we need to clear en-pessant manually as the capture square is different from the target square */
        if (move.takeEnPessant()) {
            gain[depth] = s_pieceValues[Pawn];
            occ &= ~core::enpessantCaptureSquare(toSquare, player);
        } else {
            const auto initialPiece = board.getTargetAtSquare(toSquare, player).value();
            gain[depth] = s_pieceValues[initialPiece];
        }

        /* subtract pawn from promotion value */
        if (promotionPiece.has_value()) {
            gain[depth] += s_pieceValues[*promotionPiece] - s_pieceValues[Pawn];
        }

        /* intial attack mask based on our "new board occupation" */
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
        /* create masks for both sides and return a mask with all attacks from both sides */
        const uint64_t knights = board.pieces[WhiteKnight] | board.pieces[BlackKnight];
        const uint64_t diagSliders = board.pieces[WhiteBishop] | board.pieces[BlackBishop] | board.pieces[WhiteQueen] | board.pieces[BlackQueen];
        const uint64_t hvSliders = board.pieces[WhiteRook] | board.pieces[BlackRook] | board.pieces[WhiteQueen] | board.pieces[BlackQueen];
        const uint64_t kings = board.pieces[WhiteKing] | board.pieces[BlackKing];

        /* NOTE: for pawn attacks, we use White's attack patterns to compute Black's as well
         * this works because the squares White pawns attack are exactly the ones Black pawns
         * could attack *from* (i.e., the inverse perspective on the board) */
        return (movegen::getPawnAttacksFromPos<PlayerWhite>(target) & board.pieces[BlackPawn])
            | (movegen::getPawnAttacksFromPos<PlayerBlack>(target) & board.pieces[WhitePawn])
            | (movegen::getKnightMoves(target) & knights)
            | (movegen::getBishopMoves(target, occ) & diagSliders)
            | (movegen::getRookMoves(target, occ) & hvSliders)
            | (movegen::getKingMoves(target) & kings);
    }

    template<Player player>
    constexpr static inline std::optional<Piece> getLeastValuableAttacker(const BitBoard& board, uint64_t attackers, uint64_t& occ)
    {
        constexpr auto pieces = player == PlayerWhite ? s_whitePieces : s_blackPieces;

        for (const auto piece : pieces) {
            uint64_t subset = attackers & occ & board.pieces[piece];
            if (subset) {
                occ &= ~utils::lsbToSquare(subset); /* clear the piece we just found */
                return piece;
            }
        }

        return std::nullopt; /* attackers must be empty */
    }

    constexpr static inline std::optional<Piece> getLeastValuableAttacker(const BitBoard& board, uint64_t attackers, uint64_t& occ, Player player)
    {
        if (player == PlayerWhite) {
            return getLeastValuableAttacker<PlayerWhite>(board, attackers, occ);
        } else {
            return getLeastValuableAttacker<PlayerBlack>(board, attackers, occ);
        }
    }

    /* piece values simplified */
    TUNABLE_CONSTEXPR(auto)
    s_pieceValues = std::to_array<int32_t>(
        {
            /* white pieces */
            spsa::seePawnValue,
            spsa::seeKnightValue,
            spsa::seeBishopValue,
            spsa::seeRookValue,
            spsa::seeQueenValue,
            s_maxScore,
            /* black pieces */
            spsa::seePawnValue,
            spsa::seeKnightValue,
            spsa::seeBishopValue,
            spsa::seeRookValue,
            spsa::seeQueenValue,
            s_maxScore,
        });
};
}
