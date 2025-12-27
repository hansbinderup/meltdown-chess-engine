#include <catch2/catch_test_macros.hpp>

#include "parsing/fen_parser.h"

#define private public
#include "search/move_picker.h"

namespace {

/* just some random crazy position with a lot of captures
 * note: all the moves in the test are valid moves in this position - also picked at random */
constexpr std::string_view s_testPosition = "r2qk1nr/pPp2bpp/2n2p2/1pbPp1B1/P3Q3/4NP2/1P1pP1PP/RN2KB1R w KQkq - 0 1";

}

TEST_CASE("Test MovePicker", "[MovePicker]")
{
    SECTION("Test all moves are picked and that they are valid")
    {
        const auto board = parsing::FenParser::parse(s_testPosition);
        REQUIRE(board.has_value());

        search::SearchTables searchTables;
        search::MovePicker<movegen::MovePseudoLegal> picker { searchTables, 0, search::PickerPhase::GenerateMoves };

        movegen::ValidMoves allMoves;
        core::getAllMoves<movegen::MovePseudoLegal>(board.value(), allMoves);

        uint16_t movesPicked = 0;
        while (const auto moveOpt = picker.pickNextMove(board.value())) {
            movesPicked++;

            const auto move = moveOpt.value();
            REQUIRE(std::ranges::find(allMoves, move) != allMoves.end());
        }

        REQUIRE(movesPicked == allMoves.count());
    }

    SECTION("Test TT move is picked first")
    {
        const auto board = parsing::FenParser::parse(s_testPosition);
        REQUIRE(board.has_value());

        search::SearchTables searchTables;
        const auto ttMove = movegen::Move::create(D5, C6, true);
        search::MovePicker<movegen::MovePseudoLegal> picker { searchTables, 0, search::PickerPhase::GenerateMoves, ttMove };

        const auto firstMove = picker.pickNextMove(board.value());
        REQUIRE(firstMove.has_value());
        REQUIRE(firstMove.value() == ttMove);
    }

    SECTION("Test capture-only mode filters quiets")
    {
        const auto board = parsing::FenParser::parse(s_testPosition);
        REQUIRE(board.has_value());

        search::SearchTables searchTables;
        search::MovePicker<movegen::MoveCapture> picker { searchTables, 0, search::PickerPhase::GenerateMoves };

        uint16_t movesPicked = 0;
        while (const auto moveOpt = picker.pickNextMove(board.value())) {
            movesPicked++;
            const auto move = moveOpt.value();
            REQUIRE(move.isCapture());
        }

        REQUIRE(movesPicked > 0);
    }

    SECTION("Test noisy mode picks good captures and promotions")
    {
        const auto board = parsing::FenParser::parse(s_testPosition);
        REQUIRE(board.has_value());

        search::SearchTables searchTables;
        search::MovePicker<movegen::MoveNoisy> picker { searchTables, 0, search::PickerPhase::GenerateMoves };

        bool foundMove = false;
        while (const auto moveOpt = picker.pickNextMove(board.value())) {
            foundMove = true;
            const auto move = moveOpt.value();
            REQUIRE(move.isNoisyMove());
        }

        REQUIRE(foundMove);
    }

    SECTION("Test counter move scoring")
    {
        const auto board = parsing::FenParser::parse(s_testPosition);
        REQUIRE(board.has_value());

        search::SearchTables searchTables;
        const auto prevMove = movegen::Move::create(D6, C5, false);
        const auto counterMove = movegen::Move::create(E4, H4, false);

        searchTables.updateCounterMoves(prevMove, counterMove);

        search::MovePicker<movegen::MovePseudoLegal> picker { searchTables, 0, search::PickerPhase::GenerateMoves, std::nullopt, prevMove };

        uint16_t i = 0;
        bool foundCounter = false;

        while (const auto moveOpt = picker.pickNextMove(board.value())) {
            const auto move = moveOpt.value();
            if (move == counterMove) {
                REQUIRE(picker.m_scores[i] == search::MovePickerOffsets::CounterMove);
                foundCounter = true;
                break;
            }

            i++;
        }

        REQUIRE(foundCounter);
    }

    SECTION("Test killer move scoring")
    {
        const auto board = parsing::FenParser::parse(s_testPosition);
        REQUIRE(board.has_value());

        search::SearchTables searchTables;
        const uint8_t ply = 2;
        const auto killerMove = movegen::Move::create(B1, C3, false);

        searchTables.updateKillerMoves(killerMove, ply);

        search::MovePicker<movegen::MovePseudoLegal> picker { searchTables, ply, search::PickerPhase::GenerateMoves };

        uint16_t i = 0;
        bool foundKiller = false;

        while (const auto moveOpt = picker.pickNextMove(board.value())) {
            const auto move = moveOpt.value();
            if (move == killerMove) {
                REQUIRE(picker.m_scores[i] == search::MovePickerOffsets::KillerMoveFirst);
                foundKiller = true;
                break;
            }

            i++;
        }

        REQUIRE(foundKiller);
    }

    SECTION("Test skip quiets in noisy mode")
    {
        const auto board = parsing::FenParser::parse(s_testPosition);
        REQUIRE(board.has_value());

        search::SearchTables searchTables;
        search::MovePicker<movegen::MoveNoisy> picker { searchTables, 0, search::PickerPhase::GenerateMoves };

        REQUIRE(picker.getSkipQuiets() == true);

        while (const auto moveOpt = picker.pickNextMove(board.value())) {
            const auto move = moveOpt.value();
            REQUIRE(move.isNoisyMove());
        }
    }

    SECTION("Test position with multiple captures")
    {
        const auto board = parsing::FenParser::parse(s_testPosition);
        REQUIRE(board.has_value());

        search::SearchTables searchTables;
        search::MovePicker<movegen::MoveCapture> picker { searchTables, 0, search::PickerPhase::GenerateMoves };

        movegen::ValidMoves captures;
        core::getAllMoves<movegen::MoveCapture>(*board, captures);

        while (const auto moveOpt = picker.pickNextMove(board.value())) {
            const auto move = moveOpt.value();
            REQUIRE(move.isCapture());
        }
    }
}
