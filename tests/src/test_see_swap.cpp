#include "evaluation/see_swap.h"
#include "parsing/fen_parser.h"
#include <catch2/catch_test_macros.hpp>

std::optional<movegen::Move> findMoveToTarget(const movegen::ValidMoves& moves, Piece piece, BoardPosition target)
{
    for (const auto move : moves) {
        if (move.piece() == piece && move.toPos() == target)
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
        engine::getAllMoves<movegen::MoveCapture>(*board, moves);
        REQUIRE(moves.count() == 1);

        Score score = evaluation::SeeSwap::run(*board, moves[0]);
        REQUIRE(score == 100);
    }

    SECTION("Test position 2 (Nxe5)")
    {
        const auto board = parsing::FenParser::parse("1k1r3q/1ppn3p/p4b2/4p3/8/P2N2P1/1PP1R1BP/2K1Q3 w");
        REQUIRE(board.has_value());

        movegen::ValidMoves moves;
        engine::getAllMoves<movegen::MoveCapture>(*board, moves);
        const auto move = findMoveToTarget(moves, WhiteKnight, E5);

        REQUIRE(move.has_value());
        REQUIRE(move->piece() == WhiteKnight);
        REQUIRE(move->toPos() == E5);

        Score score = evaluation::SeeSwap::run(*board, *move);
        REQUIRE(score == -322);
    }
}
