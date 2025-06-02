#include "evaluation/static_evaluation.h"
#include "parsing/fen_parser.h"

#include <catch2/catch_test_macros.hpp>

#define private public
#include "evaluation/evaluator.h"

using namespace search;

constexpr auto getPiece(const BitBoard& board, movegen::Move move)
{
    return board.getAttackerAtSquare(move.fromSquare(), board.player);
}

TEST_CASE("Scoring", "[scoring]")
{
    core::TranspositionTable::setSizeMb(16);

    SECTION("Test flipped score")
    {
        SECTION("Test start position")
        {
            BitBoard boardW;
            BitBoard boardB;

            boardW.reset();
            boardB.reset();
            boardB.player = PlayerBlack; /* flip side */

            REQUIRE(evaluation::staticEvaluation(boardW) == evaluation::staticEvaluation(boardB));
        }

        SECTION("Position 1")
        {
            const auto boardW = parsing::FenParser::parse("1k1r3q/1ppn3p/p4b2/4p3/8/P2N2P1/1PP1R1BP/2K1Q3 w");
            const auto boardB = parsing::FenParser::parse("2k1q3/1pp1r1bp/p2n2p1/8/4P3/P4B2/1PPN3P/1K1R3Q b");

            REQUIRE(boardW.has_value());
            REQUIRE(boardB.has_value());

            REQUIRE(evaluation::staticEvaluation(*boardW) == evaluation::staticEvaluation(*boardB));
        }
    }

    SECTION("Test move ordering")
    {
        evaluation::Evaluator s_evaluator;
        s_evaluator.reset();

        SECTION("Test capture moves only")
        {
            const auto board = parsing::FenParser::parse("1k6/8/8/4q3/3P4/8/n5n1/R6K w - - 0 0");
            REQUIRE(board.has_value());

            movegen::ValidMoves moves;
            core::getAllMoves<movegen::MoveCapture>(*board, moves);

            movegen::ValidMoves results {};
            SearchTables searchTables {};
            MovePicker picker {};

            while (const auto moveOpt = picker.pickNextMove(*board, searchTables, moves, 0)) {
                results.addMove(moveOpt.value());
            }

            REQUIRE(results.count() == 3);
            REQUIRE(getPiece(*board, results[0]) == WhitePawn);
            REQUIRE(getPiece(*board, results[1]) == WhiteKing);
            REQUIRE(getPiece(*board, results[2]) == WhiteRook);

            // Reset nullified moves
            core::getAllMoves<movegen::MoveCapture>(*board, moves);

            const auto move = s_evaluator.getBestMove(board.value(), 4);
            REQUIRE(getPiece(*board, move) == WhitePawn);
        }

        SECTION("Test capture and evasion moves")
        {
            const auto board = parsing::FenParser::parse("1k6/6p1/7Q/4q3/3P4/8/n5n1/R6K w - - 0 0");
            REQUIRE(board.has_value());

            movegen::ValidMoves moves;
            core::getAllMoves<movegen::MovePseudoLegal>(*board, moves);

            movegen::ValidMoves results {};
            SearchTables searchTables {};
            MovePicker picker {};

            while (const auto moveOpt = picker.pickNextMove(*board, searchTables, moves, 0)) {
                results.addMove(moveOpt.value());
            }

            REQUIRE(getPiece(*board, results[0]) == WhitePawn);
            REQUIRE(getPiece(*board, results[1]) == WhiteKing);
            REQUIRE(getPiece(*board, results[2]) == WhiteRook);

            // Reset nullified moves
            core::getAllMoves<movegen::MovePseudoLegal>(*board, moves);

            const auto move = s_evaluator.getBestMove(board.value(), 4);
            REQUIRE(getPiece(*board, move) == WhiteQueen); // evading attack + checking king = better move!
        }
    }
}
