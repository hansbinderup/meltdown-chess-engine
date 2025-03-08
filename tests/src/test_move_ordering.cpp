

#include "parsing/fen_parser.h"

#include <catch2/catch_test_macros.hpp>

#define private public
#include <evaluation/evaluator.h>

TEST_CASE("Movegen Hashing", "[move ordering]")
{
    evaluation::Evaluator s_evaluator;
    s_evaluator.reset();

    SECTION("Test capture moves only")
    {
        const auto board = parsing::FenParser::parse("1k6/8/8/4q3/3P4/8/n5n1/R6K w - - 0 0");
        REQUIRE(board.has_value());

        auto allMoves = engine::getAllMoves(board.value());
        s_evaluator.sortMoves(board.value(), allMoves, 0);

        REQUIRE(allMoves[0].piece() == WhitePawn);
        REQUIRE(allMoves[1].piece() == WhiteRook);
        REQUIRE(allMoves[2].piece() == WhiteKing);

        const auto move = s_evaluator.getBestMove(board.value(), 4);
        REQUIRE(move.piece() == WhitePawn);

        s_evaluator.sortMoves(board.value(), allMoves, 0);

        REQUIRE(allMoves[0].piece() == WhitePawn);
        REQUIRE(allMoves[1].piece() == WhiteRook);
        REQUIRE(allMoves[2].piece() == WhiteKing);
    }

    SECTION("Test capture and evasion moves")
    {
        const auto board = parsing::FenParser::parse("1k6/6p1/7Q/4q3/3P4/8/n5n1/R6K w - - 0 0");
        REQUIRE(board.has_value());

        auto allMoves = engine::getAllMoves(board.value());
        s_evaluator.sortMoves(board.value(), allMoves, 0);

        REQUIRE(allMoves[0].piece() == WhitePawn); // pawn is attacking queen
        REQUIRE(allMoves[1].piece() == WhiteRook);
        REQUIRE(allMoves[2].piece() == WhiteKing);
        REQUIRE(allMoves[3].piece() == WhiteQueen);

        const auto move = s_evaluator.getBestMove(board.value(), 4);
        REQUIRE(move.piece() == WhiteQueen); // evading attack + checking king = better move!
    }
}
