#include "src/evaluation/pv_table.h"
#include "src/movement/move_types.h"

#include <catch2/catch_test_macros.hpp>

using namespace heuristic;
using namespace movement;

void prepareTableLength(PVTable& table, uint8_t ply)
{
    for (uint8_t p = 0; p <= ply + 1; p++) {
        table.updateLength(p);
    }
}

TEST_CASE("Test PVTable heuristic", "[PVTable]")
{
    PVTable pvTable;
    prepareTableLength(pvTable, 1); // most tests are just one deep

    SECTION("PVTable: Best move retrieval", "[PVTable]")
    {
        Move move = Move::create(E2, E4, WhitePawn, false);

        pvTable.updateTable(move, 0);

        REQUIRE(pvTable.bestMove() == move);
    }

    SECTION("PVTable: Updating and retrieving moves", "[PVTable]")
    {
        uint8_t size = G3 + 1;
        prepareTableLength(pvTable, size);

        // NOTE: moves have to be added in descending order as we're creating the PV nodes
        // This makes sense as we don't add any PV nodes UNTILL we've searched as deep as we need,
        // and THEN we will start adding moves
        for (int8_t p = G3; p >= A1; p--) {
            Move move = Move::create(p, p + 8, WhitePawn, false);
            pvTable.updateTable(move, p);
        }

        REQUIRE(pvTable.size() == size);
        for (uint8_t p = A1; p <= G3; p++) {
            Move move = Move::create(p, p + 8, WhitePawn, false);
            REQUIRE(pvTable[p] == move);
        }
    }

    SECTION("PVTable: Checking PV move status", "[PVTable]")
    {
        Move move = Move::create(B1, C3, WhiteKnight, false);

        pvTable.updateTable(move, 0);

        REQUIRE(pvTable.isPvMove(move, 0) == true);
    }

    SECTION("PVTable: Reset functionality", "[PVTable]")
    {
        Move move = Move::create(E7, E5, WhitePawn, false);

        pvTable.updateTable(move, 0);
        REQUIRE(pvTable.bestMove() == move);

        pvTable.reset();
        REQUIRE(pvTable.bestMove() == Move {});
    }

    SECTION("PVTable: Following and scoring flags", "[PVTable]")
    {
        pvTable.setIsFollowing(true);
        REQUIRE(pvTable.isFollowing() == true);

        pvTable.setIsScoring(true);
        REQUIRE(pvTable.isScoring() == true);

        pvTable.setIsFollowing(false);
        REQUIRE(pvTable.isFollowing() == false);
    }

    SECTION("PVTable: Updating PV scoring", "[PVTable]")
    {
        Move move = Move::create(H2, H4, WhitePawn, false);
        ValidMoves validMoves;

        validMoves.addMove(move);
        pvTable.updateTable(move, 0);

        REQUIRE(pvTable.isScoring() == false);
        pvTable.updatePvScoring(validMoves, 0);
        REQUIRE(pvTable.isScoring() == true);
    }
}

