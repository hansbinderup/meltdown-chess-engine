#include "parsing/fen_parser.h"
#include <catch2/catch_test_macros.hpp>

#define private public
#include <tools/perft.h>

/* results found here: https://www.chessprogramming.org/Perft_Results */

using namespace tools;

TEST_CASE("Perft", "[perft]")
{
    core::TranspositionTable::setSizeMb(16);

    SECTION("Test perft from start position")
    {
        const auto board = parsing::FenParser::parse(s_startPosFen);
        REQUIRE(board.has_value());

        Perft::run(board.value(), 5);
        REQUIRE(Perft::s_nodes == 4865609);
        REQUIRE(Perft::s_captures == 82719);
        REQUIRE(Perft::s_enPessants == 258);
        REQUIRE(Perft::s_promotions == 0);
        REQUIRE(Perft::s_castles == 0);
        REQUIRE(Perft::s_checks == 27351);
        REQUIRE(Perft::s_checkMates == 8);
    }

    SECTION("Test perft from tricky position (Kiwipete)")
    {
        const auto fenBoard = parsing::FenParser::parse("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 0");
        REQUIRE(fenBoard.has_value());

        Perft::run(fenBoard.value(), 4);
        REQUIRE(Perft::s_nodes == 4085603);
        REQUIRE(Perft::s_captures == 757163);
        REQUIRE(Perft::s_enPessants == 1929);
        REQUIRE(Perft::s_promotions == 15172);
        REQUIRE(Perft::s_castles == 128013);
        REQUIRE(Perft::s_checks == 25523);
        REQUIRE(Perft::s_checkMates == 1);
    }
}
