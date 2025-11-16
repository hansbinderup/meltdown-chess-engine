#include "evaluation/static_evaluation.h"
#include "parsing/fen_parser.h"

#include <catch2/catch_test_macros.hpp>

#define private public
#include "evaluation/evaluator.h"

using namespace search;
using namespace evaluation;

constexpr auto getPiece(const BitBoard& board, movegen::Move move)
{
    return board.getAttackerAtSquare(move.fromSquare(), board.player);
}

TEST_CASE("Scoring", "[scoring]")
{
    core::TranspositionTable::setSizeMb(16);
    StaticEvaluation staticEval {};

    SECTION("Test flipped score")
    {
        SECTION("Test start position")
        {
            auto boardW = parsing::FenParser::parse(s_startPosFen);
            auto boardB = parsing::FenParser::parse(s_startPosFen);

            REQUIRE(boardW.has_value());
            REQUIRE(boardB.has_value());

            boardB->player = PlayerBlack; /* flip side */

            REQUIRE(staticEval.get(*boardW) == staticEval.get(*boardB));
        }

        SECTION("Position 1")
        {
            const auto boardW = parsing::FenParser::parse("1k1r3q/1ppn3p/p4b2/4p3/8/P2N2P1/1PP1R1BP/2K1Q3 w");
            const auto boardB = parsing::FenParser::parse("2k1q3/1pp1r1bp/p2n2p1/8/4P3/P4B2/1PPN3P/1K1R3Q b");

            REQUIRE(boardW.has_value());
            REQUIRE(boardB.has_value());

            REQUIRE(staticEval.get(*boardW) == staticEval.get(*boardB));
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

            movegen::ValidMoves results {};
            SearchTables searchTables {};
            MovePicker<movegen::MoveCapture> picker { searchTables, 0, PickerPhase::GenerateMoves };

            while (const auto moveOpt = picker.pickNextMove(*board)) {
                results.addMove(moveOpt.value());
            }

            REQUIRE(results.count() == 3);
            REQUIRE(getPiece(*board, results[0]) == Pawn);
            REQUIRE(getPiece(*board, results[1]) == King);
            REQUIRE(getPiece(*board, results[2]) == Rook);

            const auto move = s_evaluator.getBestMove(board.value(), 4);
            REQUIRE(getPiece(*board, move) == Pawn);
        }

        SECTION("Test capture and evasion moves")
        {
            const auto board = parsing::FenParser::parse("1k6/6p1/7Q/4q3/3P4/8/n5n1/R6K w - - 0 0");
            REQUIRE(board.has_value());

            movegen::ValidMoves results {};
            SearchTables searchTables {};
            MovePicker<movegen::MovePseudoLegal> picker { searchTables, 0, PickerPhase::GenerateMoves };

            while (const auto moveOpt = picker.pickNextMove(*board)) {
                results.addMove(moveOpt.value());
            }

            REQUIRE(getPiece(*board, results[0]) == Pawn);
            REQUIRE(getPiece(*board, results[1]) == King);
            REQUIRE(getPiece(*board, results[2]) == Rook);

            const auto move = s_evaluator.getBestMove(board.value(), 4);
            REQUIRE(getPiece(*board, move) == Queen); // evading attack + checking king = better move!
        }
    }
}
