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
    static inline bool isGreaterThanMargin(const BitBoard& board, movegen::Move move, int32_t margin)
    {
        if (move.isCastleMove()) {
            return margin >= 0;
        }

        /* as we're comparing with margin we can simplify handling by comparing to zero instead */
        int32_t balance = -margin;

        Player player = board.player;
        const uint64_t fromSquare = move.fromSquare();
        const uint64_t toSquare = move.toSquare();

        /* the position we are operation on - all attacks will be targeted here */
        const BoardPosition target = move.toPos();

        const auto promotionPiece = promotionToPiece(move.promotionType());

        /* piece that will track the scoring of next piece */
        auto nextPiece = promotionPiece.has_value()
            ? promotionPiece.value()
            : board.getAttackerAtSquare(fromSquare, board.player).value();

        /* remove our current move's piece - it's "assumed" to already have been moved to the target square */
        uint64_t occ = (board.occupation[Both] & ~fromSquare) | toSquare;

        if (move.takeEnPessant()) {
            occ &= ~core::enpessantCaptureSquare(toSquare, board.player);
            balance += s_pieceValues[Pawn];
        } else if (move.isCapture()) {
            const auto target = board.getTargetAtSquare(toSquare, board.player).value();
            balance += s_pieceValues[target];
        }

        if (promotionPiece.has_value()) {
            balance += s_pieceValues[*promotionPiece] - s_pieceValues[Pawn];
        }

        /* intial attack mask based on our "new board occupation" */
        uint64_t attackers = getAttackers(board, target, occ) & occ;

        while (true) {
            player = nextPlayer(player);
            if ((player == board.player && balance >= 0) || (player != board.player && balance <= 0)) {
                break;
            }

            const auto piece = getLeastValuableAttacker(board, attackers, occ, player);
            if (!piece.has_value())
                break; /* no more attackers */

            /* illegal position if king is attacked after capturing */
            if (*piece == King && ((attackers & occ & board.occupation[nextPlayer(board.player)]) != 0)) {
                break;
            }

            if (player == board.player) {
                balance += s_pieceValues[nextPiece];
            } else {
                balance -= s_pieceValues[nextPiece];
            }

            /* update attackers after removing captured piece */
            attackers = getAttackers(board, target, occ) & occ;
            nextPiece = *piece;
        }

        return balance >= 0;
    }

    /* FIXME: use capture history as margin and replace with above */
    static inline int32_t getCaptureScore(const BitBoard& board, movegen::Move move)
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

        const auto promotionPiece = promotionToPiece(move.promotionType());

        /* piece that will track the scoring of next piece */
        auto nextPiece = promotionPiece.has_value()
            ? promotionPiece.value()
            : board.getAttackerAtSquare(fromSquare, board.player).value();

        /* remove our current move's piece - it's "assumed" to already have been moved to the target square */
        uint64_t occ = (board.occupation[Both] & ~fromSquare) | toSquare;

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
        uint64_t attackers = getAttackers(board, target, occ) & occ;

        while (true) {
            depth++;

            player = nextPlayer(player);
            const auto piece = getLeastValuableAttacker(board, attackers, occ, player);
            if (!piece.has_value())
                break; // No more attackers

            /* update speculative score */
            gain[depth] = s_pieceValues[nextPiece] - gain[depth - 1];
            nextPiece = *piece;

            /* update attackers after removing captured piece */
            attackers = getAttackers(board, target, occ) & occ;
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
        const uint64_t diagSliders = board.pieces[Bishop] | board.pieces[Queen];
        const uint64_t hvSliders = board.pieces[Rook] | board.pieces[Queen];

        /* NOTE: for pawn attacks, we use White's attack patterns to compute Black's as well
         * this works because the squares White pawns attack are exactly the ones Black pawns
         * could attack *from* (i.e., the inverse perspective on the board) */
        return (movegen::getPawnAttacksFromPos<PlayerWhite>(target) & board.pieces[Pawn] & board.occupation[PlayerBlack])
            | (movegen::getPawnAttacksFromPos<PlayerBlack>(target) & board.pieces[Pawn] & board.occupation[PlayerWhite])
            | (movegen::getKnightMoves(target) & board.pieces[Knight])
            | (movegen::getBishopMoves(target, occ) & diagSliders)
            | (movegen::getRookMoves(target, occ) & hvSliders)
            | (movegen::getKingMoves(target) & board.pieces[King]);
    }

    template<Player player>
    constexpr static inline std::optional<Piece> getLeastValuableAttacker(const BitBoard& board, uint64_t attackers, uint64_t& occ)
    {
        for (const auto piece : magic_enum::enum_values<Piece>()) {
            uint64_t subset = attackers & board.pieces[piece] & board.occupation[player];
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
    s_pieceValues = std::to_array<int32_t>({
        spsa::seePawnValue,
        spsa::seeKnightValue,
        spsa::seeBishopValue,
        spsa::seeRookValue,
        spsa::seeQueenValue,
        s_maxScore,
    });
};
}
