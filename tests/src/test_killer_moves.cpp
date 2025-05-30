#include "movegen/move_types.h"
#include "search/killer_moves.h"

#include <catch2/catch_test_macros.hpp>

using namespace search;
using namespace movegen;

constexpr uint8_t g_testPly = 5;

TEST_CASE("KillerMoves: Updating with quiet moves", "[KillerMoves]")
{
    KillerMoves killerMoves;

    Move quietMove1 = Move::create(12, 28, false);
    Move quietMove2 = Move::create(20, 36, false);

    REQUIRE(killerMoves.get(g_testPly) == KillerMoves::KillerMove {});

    killerMoves.update(quietMove1, g_testPly);
    REQUIRE(killerMoves.get(g_testPly) == KillerMoves::KillerMove { quietMove1, {} });

    killerMoves.update(quietMove2, g_testPly);
    REQUIRE(killerMoves.get(g_testPly) == KillerMoves::KillerMove { quietMove2, quietMove1 });
}

TEST_CASE("KillerMoves: Ignoring capture moves", "[KillerMoves]")
{
    KillerMoves killerMoves;

    Move captureMove = Move::create(10, 26, true);
    Move quietMove = Move::create(14, 30, false);

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

    Move quietMove1 = Move::create(8, 24, false);
    Move quietMove2 = Move::create(16, 32, false);

    killerMoves.update(quietMove1, g_testPly);
    killerMoves.update(quietMove2, g_testPly);
    REQUIRE(killerMoves.get(g_testPly) == KillerMoves::KillerMove { quietMove2, quietMove1 });

    killerMoves.reset();
    REQUIRE(killerMoves.get(g_testPly) == KillerMoves::KillerMove {});
}

