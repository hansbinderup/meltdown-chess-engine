
#include "bit_board.h"
#include "engine/move_handling.h"
#include "parsing/fen_parser.h"
#include <catch2/catch_test_macros.hpp>

constexpr uint8_t s_defaultSearchDepth = 3;

void testAllMoves(const BitBoard& board, uint8_t depth = s_defaultSearchDepth)
{
    const uint64_t originalHash = engine::generateHashKey(board);

    movegen::ValidMoves moves;
    engine::getAllMoves<movegen::MovePseudoLegal>(board, moves);
    for (const auto& move : moves) {
        uint64_t hash = originalHash;
        const auto newBoard = engine::performMove(board, move, hash);
        const auto newHash = engine::generateHashKey(newBoard);

        REQUIRE(hash == newHash);

        if (depth != 0) {
            testAllMoves(newBoard, depth - 1);
        }
    }
}

TEST_CASE("Movegen Hashing", "[movegen]")
{
    engine::TtHashTable::setSizeMb(16);

    SECTION("Test from start position")
    {
        BitBoard board;
        board.reset(); // start position

        /* go to depth 4 to capture enpessant moves */
        testAllMoves(board, 4);
    }

    SECTION("Test from tricky castle position")
    {
        const auto board = parsing::FenParser::parse("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 0");
        REQUIRE(board.has_value());

        testAllMoves(board.value());
    }

    SECTION("Test from promotion position")
    {
        const auto board = parsing::FenParser::parse("3k2n1/PP4PP/2P2P2/8/8/1p6/p6p/1N1K4 w - - 0 0");
        REQUIRE(board.has_value());

        testAllMoves(board.value());
    }
}
