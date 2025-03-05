
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>

#define private public
#include <src/engine/tt_hash_table.h>

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

    TtHashTable::writeEntry(key, score, depth, ply, TtHashExact);
    auto retrieved = TtHashTable::TtHashTable::readEntry(key, alpha, beta, depth, ply);

    REQUIRE(retrieved.has_value());
    REQUIRE(retrieved.value() == score);
}

TEST_CASE("Transposition Table - Depth Overwrite Rule", "[TT]")
{
    TtHashTable::clear();

    uint64_t key = 0xDEADBEEFCAFEBABE;
    int32_t oldScore = 10;
    int32_t newScore = 50;

    TtHashTable::writeEntry(key, oldScore, 3, ply, TtHashExact);
    TtHashTable::writeEntry(key, newScore, 6, ply, TtHashExact); // Deeper depth

    auto retrieved = TtHashTable::readEntry(key, alpha, beta, 4, ply);
    REQUIRE(retrieved.has_value());
    REQUIRE(retrieved.value() == newScore);
}

TEST_CASE("Transposition Table - Collision Handling", "[TT]")
{
    TtHashTable::clear();

    uint64_t key1 = 0x1111111111111111;
    uint64_t key2 = key1 + TtHashTable::s_ttHashSize; // Forces collision

    TtHashTable::writeEntry(key1, 30, depth, ply, TtHashExact);
    TtHashTable::writeEntry(key2, 99, depth, ply, TtHashExact);

    auto retrieved1 = TtHashTable::readEntry(key1, alpha, beta, depth, ply);
    auto retrieved2 = TtHashTable::readEntry(key2, alpha, beta, depth, ply);

    REQUIRE_FALSE(retrieved1.has_value());

    REQUIRE(retrieved2.has_value());
    REQUIRE(retrieved2.value() == 99);
}

TEST_CASE("Transposition Table - Score Clamping for Mating Scores", "[TT]")
{
    TtHashTable::clear();

    constexpr int32_t nearMate = s_mateValue - 10;

    uint64_t key = 0xCAFEBABEDEADBEEF;

    TtHashTable::writeEntry(key, nearMate, depth, ply, TtHashExact);

    SECTION("From same ply")
    {
        auto retrieved = TtHashTable::readEntry(key, alpha, beta, depth, ply);

        REQUIRE(retrieved.has_value());
        REQUIRE(retrieved.value() == nearMate); // same score
    }

    SECTION("From higher ply")
    {
        uint8_t plyOffset = 5;
        auto retrieved = TtHashTable::readEntry(key, alpha, beta, depth, ply + plyOffset);

        REQUIRE(retrieved.has_value());
        REQUIRE(retrieved.value() == nearMate - plyOffset); // lower score
    }

    SECTION("From lower ply")
    {
        uint8_t plyOffset = 1;
        auto retrieved = TtHashTable::readEntry(key, alpha, beta, depth, ply - plyOffset);

        REQUIRE(retrieved.has_value());
        REQUIRE(retrieved.value() == nearMate + plyOffset); // higher score
    }
}

TEST_CASE("Transposition Table - Clear Functionality", "[TT]")
{
    TtHashTable::clear();

    uint64_t key = 0xF00DBABE12345678;
    TtHashTable::writeEntry(key, 77, depth, ply, TtHashExact);

    TtHashTable::clear();

    auto retrieved = TtHashTable::readEntry(key, alpha, beta, depth, ply);
    REQUIRE_FALSE(retrieved.has_value()); // Should be cleared
}

