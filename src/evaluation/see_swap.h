#pragma once

#include "bit_board.h"
#include "board_defs.h"

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
    static inline int32_t run(const BitBoard& board, const movegen::Move& move)
    {
        if (move.isCastleMove()) {
            return 0;
        } else if (move.hasEnPessant()) {
            return 100; /* Pawn takes pawn, so give it pawn score - anything else is difficult to handle here */
        }

        /* remove our current move's piece - it's "assumed" to already have been moved to the target square */
        uint64_t occ = board.occupation[Both] & ~(move.fromSquare());

        /* the position we are operation on - all attacks will be targeted here */
        const BoardPosition target = move.toPos();

        /* intial attack mask based on our "new board occupation" */
        uint64_t attackers = getAttackers<PlayerWhite>(board, target, occ) | getAttackers<PlayerBlack>(board, target, occ);

        if (attackers == 0)
            return 0; // No attackers, no exchanges

        int depth = 0;
        std::array<int32_t, 32> gain {}; // Stores gains for each exchange depth

        /* piece that will track the scoring of next piece */
        Piece nextPiece = move.piece();

        Player player = board.player;

        if (const auto initialPiece = board.getTargetAtSquare(move.toSquare(), board.player)) {
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
            attackers = getAttackers<PlayerWhite>(board, target, occ) | getAttackers<PlayerBlack>(board, target, occ);
        }

        /* Backtrack the sequence and evaluate the score */
        while (--depth > 0) {
            gain[depth - 1] = -std::max(-gain[depth - 1], gain[depth]);
        }

        return gain[0];
    }

private:
    template<Player player>
    constexpr static inline uint64_t getAttackers(const BitBoard& board, BoardPosition target, uint64_t occ)
    {
        if constexpr (player == PlayerWhite) {
            return s_whitePawnTable[target]
                | (movegen::getKnightMoves(target) & board.pieces[WhiteKnight])
                | (movegen::getBishopMoves(target, occ) & (board.pieces[WhiteBishop] | board.pieces[WhiteQueen]))
                | (movegen::getRookMoves(target, occ) & (board.pieces[WhiteRook] | board.pieces[WhiteQueen]))
                | (movegen::getKingMoves(target) & board.pieces[WhiteKing]);
        } else {
            return s_blackPawnTable[target]
                | (movegen::getKnightMoves(target) & board.pieces[BlackKnight])
                | (movegen::getBishopMoves(target, occ) & (board.pieces[BlackBishop] | board.pieces[BlackQueen]))
                | (movegen::getRookMoves(target, occ) & (board.pieces[BlackRook] | board.pieces[BlackQueen]))
                | (movegen::getKingMoves(target) & board.pieces[BlackKing]);
        }
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

    /* piece values simplified */
    constexpr static inline auto s_pieceValues = std::to_array<int32_t>(
        { /* white pieces */
            100, 422, 422, 642, 1015, 30000,
            /* black pieces */
            100, 422, 422, 642, 1025, 30000 });
};
}
