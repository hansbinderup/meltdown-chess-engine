
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>

#define private public
#include <engine/tt_hash_table.h>

using namespace engine;

constexpr int32_t alpha = -100;
constexpr int32_t beta = 100;
constexpr uint8_t depth = 5;
constexpr uint8_t ply = 2;

TEST_CASE("Transposition Table - Basic Write & Read", "[TT]")
{
    TtHashTable::TtHashTable::clear();

    uint64_t key = 0x123456789ABCDEF;
    int32_t score = 42;

    const auto move = movement::Move::create(A2, A4, WhitePawn, false);

    TtHashTable::writeEntry(key, score, move, depth, ply, TtHashExact);
    auto retrieved = TtHashTable::TtHashTable::probe(key, alpha, beta, depth, ply);

    REQUIRE(retrieved.has_value());
    REQUIRE(retrieved->first == score);
    REQUIRE(retrieved->second == move);
}

TEST_CASE("Transposition Table - Depth Overwrite Rule", "[TT]")
{
    TtHashTable::clear();

    uint64_t key = 0xDEADBEEFCAFEBABE;
    int32_t oldScore = 10;
    int32_t newScore = 50;

    const auto move1 = movement::Move::create(A2, A4, WhitePawn, false);
    const auto move2 = movement::Move::create(D7, D7, BlackPawn, false);

    TtHashTable::writeEntry(key, oldScore, move1, 3, ply, TtHashExact);
    TtHashTable::writeEntry(key, newScore, move2, 6, ply, TtHashExact); // Deeper depth

    auto retrieved = TtHashTable::probe(key, alpha, beta, 4, ply);
    REQUIRE(retrieved.has_value());
    REQUIRE(retrieved->first == newScore);
    REQUIRE(retrieved->second == move2);
}

TEST_CASE("Transposition Table - Collision Handling", "[TT]")
{
    TtHashTable::clear();

    uint64_t key1 = 0x1111111111111111;
    uint64_t key2 = key1 + TtHashTable::s_ttHashSize; // Forces collision

    const auto move1 = movement::Move::create(A2, A4, WhitePawn, false);
    const auto move2 = movement::Move::create(D7, D7, BlackPawn, false);

    TtHashTable::writeEntry(key1, 30, move1, depth, ply, TtHashExact);
    TtHashTable::writeEntry(key2, 99, move2, depth, ply, TtHashExact);

    auto retrieved1 = TtHashTable::probe(key1, alpha, beta, depth, ply);
    auto retrieved2 = TtHashTable::probe(key2, alpha, beta, depth, ply);

    REQUIRE_FALSE(retrieved1.has_value());

    REQUIRE(retrieved2.has_value());
    REQUIRE(retrieved2->first == 99);
    REQUIRE(retrieved2->second == move2);
}

TEST_CASE("Transposition Table - Score Clamping for Mating Scores", "[TT]")
{
    TtHashTable::clear();

    constexpr int32_t nearMate = s_mateValue - 10;

    uint64_t key = 0xCAFEBABEDEADBEEF;

    const auto move = movement::Move::create(A2, A4, WhitePawn, false);
    TtHashTable::writeEntry(key, nearMate, move, depth, ply, TtHashExact);

    SECTION("From same ply")
    {
        auto retrieved = TtHashTable::probe(key, alpha, beta, depth, ply);

        REQUIRE(retrieved.has_value());
        REQUIRE(retrieved->first == nearMate); // same score
        REQUIRE(retrieved->second == move);
    }

    SECTION("From higher ply")
    {
        uint8_t plyOffset = 5;
        auto retrieved = TtHashTable::probe(key, alpha, beta, depth, ply + plyOffset);

        REQUIRE(retrieved.has_value());
        REQUIRE(retrieved->first == nearMate - plyOffset); // lower score
        REQUIRE(retrieved->second == move);
    }

    SECTION("From lower ply")
    {
        uint8_t plyOffset = 1;

        auto retrieved = TtHashTable::probe(key, alpha, beta, depth, ply - plyOffset);

        REQUIRE(retrieved.has_value());
        REQUIRE(retrieved->first == nearMate + plyOffset); // higher score
        REQUIRE(retrieved->second == move);
    }
}

TEST_CASE("Transposition Table - Clear Functionality", "[TT]")
{
    TtHashTable::clear();

    uint64_t key = 0xF00DBABE12345678;
    const auto move = movement::Move::create(A2, A4, WhitePawn, false);
    TtHashTable::writeEntry(key, 77, move, depth, ply, TtHashExact);

    TtHashTable::clear();

    auto retrieved = TtHashTable::probe(key, alpha, beta, depth, ply);
    REQUIRE_FALSE(retrieved.has_value()); // Should be cleared
}

