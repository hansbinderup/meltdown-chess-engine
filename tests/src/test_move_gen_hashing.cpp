
#include "core/bit_board.h"
#include "core/move_handling.h"
#include "parsing/fen_parser.h"
#include <catch2/catch_test_macros.hpp>

constexpr uint8_t s_defaultSearchDepth = 3;

void testAllMoves(const BitBoard& board, uint8_t depth = s_defaultSearchDepth)
{
    REQUIRE(board.hash == core::generateHash(board));
    REQUIRE(board.kpHash == core::generateKingPawnHash(board));

    movegen::ValidMoves moves;
    core::getAllMoves<movegen::MovePseudoLegal>(board, moves);
    for (const auto& move : moves) {
        const auto newBoard = core::performMove(board, move);

        /* don't end up in positions where eg king is gone */
        if (core::isKingAttacked(newBoard, board.player)) {
            continue;
        }

        REQUIRE(newBoard.hash == core::generateHash(newBoard));
        REQUIRE(newBoard.kpHash == core::generateKingPawnHash(newBoard));

        if (depth != 0) {
            testAllMoves(newBoard, depth - 1);
        }
    }
}

TEST_CASE("Movegen Hashing", "[movegen]")
{
    core::TranspositionTable::setSizeMb(16);

    SECTION("Test from start position")
    {
        const auto board = parsing::FenParser::parse(s_startPosFen);
        REQUIRE(board.has_value());

        /* go to depth 4 to capture enpessant moves */
        testAllMoves(board.value(), 4);
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
