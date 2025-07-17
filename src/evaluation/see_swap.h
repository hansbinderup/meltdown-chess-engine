#pragma once

#include "core/bit_board.h"
#include "core/board_defs.h"

#include "movegen/bishops.h"
#include "movegen/kings.h"
#include "movegen/knights.h"
#include "movegen/move_types.h"
#include "movegen/rooks.h"
#include "spsa/parameters.h"

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
        if (move.isCastleMove()) {
            return 0;
        } else if (move.isDoublePush()) {
            /* FIXME: should be take enpessant? */
            return 100; /* Pawn takes pawn, so give it pawn score - anything else is difficult to handle here */
        }

        /* remove our current move's piece - it's "assumed" to already have been moved to the target square */
        uint64_t occ = board.occupation[Both] & ~(move.fromSquare());

        /* the position we are operation on - all attacks will be targeted here */
        const BoardPosition target = move.toPos();

        /* intial attack mask based on our "new board occupation" */
        uint64_t attackers = getAttackers(board, target, occ);

        if (attackers == 0)
            return 0; // No attackers, no exchanges

        int depth = 0;
        std::array<int32_t, 32> gain {}; // Stores gains for each exchange depth

        /* piece that will track the scoring of next piece
         * NOTE: WhiteQueen is just a hack - white and black have same score */
        Piece nextPiece = move.promotionType() == PromotionQueen ? WhiteQueen : board.getAttackerAtSquare(move.fromSquare(), board.player).value();

        Player player = board.player;

        if (const auto initialPiece = board.getTargetAtSquare(move.toSquare(), player)) {
            gain[depth] = s_pieceValues[*initialPiece];
        }

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

        return (s_whitePawnTable[target] & board.pieces[WhitePawn])
            | (s_blackPawnTable[target] & board.pieces[BlackPawn])
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

    constexpr static inline auto s_whitePawnTable = generatePawnTable<PlayerWhite>();
    constexpr static inline auto s_blackPawnTable = generatePawnTable<PlayerBlack>();

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
