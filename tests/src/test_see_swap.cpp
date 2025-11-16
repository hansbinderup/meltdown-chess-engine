#include "evaluation/see_swap.h"
#include "parsing/fen_parser.h"
#include <catch2/catch_test_macros.hpp>

constexpr auto getPiece(const BitBoard& board, movegen::Move move)
{
    return board.getAttackerAtSquare(move.fromSquare(), board.player);
}

std::optional<movegen::Move> findMoveToTarget(const BitBoard& board, const movegen::ValidMoves& moves, Piece piece, BoardPosition target)
{
    for (const auto move : moves) {
        if (getPiece(board, move) == piece && move.toPos() == target)
            return move;
    }

    return std::nullopt;
}

/* positions from https://www.chessprogramming.org/SEE_-_The_Swap_Algorithm
 * NOTE: scores are based on our tuning in the SeeSwap "pieceValues" */
TEST_CASE("Test See Swap", "[SeeSwap]")
{

    SECTION("Test position 1")
    {
        const auto board = parsing::FenParser::parse("1k1r4/1pp4p/p7/4p3/8/P5P1/1PP4P/2K1R3 w");
        REQUIRE(board.has_value());

        movegen::ValidMoves moves;
        core::getAllMoves<movegen::MoveCapture>(*board, moves);
        REQUIRE(moves.count() == 1);

        Score score = evaluation::SeeSwap::getCaptureScore(*board, moves[0]);
        REQUIRE(score == spsa::seePawnValue);
    }

    SECTION("Test position 2 (Nxe5)")
    {
        const auto board = parsing::FenParser::parse("1k1r3q/1ppn3p/p4b2/4p3/8/P2N2P1/1PP1R1BP/2K1Q3 w");
        REQUIRE(board.has_value());

        movegen::ValidMoves moves;
        core::getAllMoves<movegen::MoveCapture>(*board, moves);
        const auto move = findMoveToTarget(*board, moves, Knight, E5);

        REQUIRE(move.has_value());
        REQUIRE(getPiece(*board, *move) == Knight);
        REQUIRE(move->toPos() == E5);

        Score score = evaluation::SeeSwap::getCaptureScore(*board, *move);
        REQUIRE(score == (spsa::seePawnValue - spsa::seeKnightValue));
    }
}
