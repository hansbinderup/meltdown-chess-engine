#include "src/evaluation/killer_moves.h"
#include "src/movement/move_types.h"

#include <catch2/catch_test_macros.hpp>

using namespace heuristic;
using namespace movement;

constexpr uint8_t g_testPly = 5;

TEST_CASE("KillerMoves: Updating with quiet moves", "[KillerMoves]")
{
    KillerMoves killerMoves;

    Move quietMove1 = Move::create(12, 28, WhiteKnight, false);
    Move quietMove2 = Move::create(20, 36, WhiteBishop, false);

    REQUIRE(killerMoves.get(g_testPly) == KillerMoves::KillerMove {});

    killerMoves.update(quietMove1, g_testPly);
    REQUIRE(killerMoves.get(g_testPly) == KillerMoves::KillerMove { quietMove1, {} });

    killerMoves.update(quietMove2, g_testPly);
    REQUIRE(killerMoves.get(g_testPly) == KillerMoves::KillerMove { quietMove2, quietMove1 });
}

TEST_CASE("KillerMoves: Ignoring capture moves", "[KillerMoves]")
{
    KillerMoves killerMoves;

    Move captureMove = Move::create(10, 26, WhitePawn, true);
    Move quietMove = Move::create(14, 30, WhiteQueen, false);

    killerMoves.update(captureMove, g_testPly);
    REQUIRE(killerMoves.get(g_testPly) == KillerMoves::KillerMove {});

    killerMoves.update(quietMove, g_testPly);
    REQUIRE(killerMoves.get(g_testPly) == KillerMoves::KillerMove { quietMove, {} });

    killerMoves.update(captureMove, g_testPly);
    REQUIRE(killerMoves.get(g_testPly) == KillerMoves::KillerMove { quietMove, {} });
}

TEST_CASE("KillerMoves: Reset functionality", "[KillerMoves]")
{
    KillerMoves killerMoves;

    Move quietMove1 = Move::create(8, 24, WhiteRook, false);
    Move quietMove2 = Move::create(16, 32, WhiteKnight, false);

    killerMoves.update(quietMove1, g_testPly);
    killerMoves.update(quietMove2, g_testPly);
    REQUIRE(killerMoves.get(g_testPly) == KillerMoves::KillerMove { quietMove2, quietMove1 });

    killerMoves.reset();
    REQUIRE(killerMoves.get(g_testPly) == KillerMoves::KillerMove {});
}

