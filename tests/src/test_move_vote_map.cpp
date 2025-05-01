#include "evaluation/move_vote_map.h"

#include <catch2/catch_test_macros.hpp>

using evaluation::MoveVoteMap;
using movegen::Move;

TEST_CASE("MoveVoteMap", "[move_vote_map]")
{

    constexpr size_t maxSize = 4;
    MoveVoteMap<maxSize> voteMap;

    SECTION("Insert new move")
    {
        Move m1(0x12345678);
        voteMap.insertOrIncrement(m1, 1);

        REQUIRE(std::distance(voteMap.begin(), voteMap.end()) == 1);
        REQUIRE(voteMap.begin()->first == m1);
        REQUIRE(voteMap.begin()->second == 1);
    }

    SECTION("Increment existing vote")
    {
        Move m1(0x12345678);
        voteMap.insertOrIncrement(m1, 2);
        voteMap.insertOrIncrement(m1, 3);

        REQUIRE(std::distance(voteMap.begin(), voteMap.end()) == 1);
        REQUIRE(voteMap.begin()->first == m1);
        REQUIRE(voteMap.begin()->second == 5);
    }

    SECTION("Insert multiple unique moves")
    {
        Move m1(0x1);
        Move m2(0x2);
        Move m3(0x3);

        voteMap.insertOrIncrement(m1, 1);
        voteMap.insertOrIncrement(m2, 2);
        voteMap.insertOrIncrement(m3, 3);

        REQUIRE(std::distance(voteMap.begin(), voteMap.end()) == 3);
    }

    SECTION("Clear resets entries")
    {
        Move m1(0x12345678);
        voteMap.insertOrIncrement(m1, 1);

        voteMap.clear();

        REQUIRE(std::distance(voteMap.begin(), voteMap.end()) == 0);
    }

    SECTION("Initializer list constructs map correctly")
    {
        Move m1(0x10), m2(0x20);
        MoveVoteMap<maxSize> initMap { { m1, 2 }, { m2, 4 } };

        REQUIRE(std::distance(initMap.begin(), initMap.end()) == 2);
        REQUIRE(initMap.begin()->first == m1);
        REQUIRE(initMap.begin()->second == 2);
        REQUIRE((initMap.begin() + 1)->first == m2);
        REQUIRE((initMap.begin() + 1)->second == 4);
    }
}
