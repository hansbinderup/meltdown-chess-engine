#include "src/parsing/fen_parser.h"
#include <catch2/catch_test_macros.hpp>

#include <src/evaluation/perft.h>

/* results found here: https://www.chessprogramming.org/Perft_Results */

TEST_CASE("Perft", "[perft]")
{
    BitBoard board;
    board.reset();

    SECTION("Test perft from start position")
    {
        Perft::run(board, 5);
        REQUIRE(Perft::getNodes() == 4865609);
    }

    SECTION("Test perft from tricky position")
    {
        const auto fenBoard = parsing::FenParser::parse("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 0");
        REQUIRE(fenBoard.has_value());

        Perft::run(fenBoard.value(), 4);
        REQUIRE(Perft::getNodes() == 4085603);
    }
}
