
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>

#include <src/engine/tt_hash.h>

using namespace engine::tt;

constexpr int16_t alpha = -100;
constexpr int16_t beta = 100;
constexpr uint8_t depth = 5;
constexpr uint8_t ply = 2;

TEST_CASE("Transposition Table - Basic Write & Read", "[TT]")
{
    clearTable();

    uint64_t key = 0x123456789ABCDEF;
    int16_t score = 42;

    writeHashEntry(key, score, depth, ply, TtHashExact);
    auto retrieved = readHashEntry(key, alpha, beta, depth, ply);

    REQUIRE(retrieved.has_value());
    REQUIRE(retrieved.value() == score);
}

TEST_CASE("Transposition Table - Depth Overwrite Rule", "[TT]")
{
    clearTable();

    uint64_t key = 0xDEADBEEFCAFEBABE;
    int16_t oldScore = 10;
    int16_t newScore = 50;

    writeHashEntry(key, oldScore, 3, ply, TtHashExact);
    writeHashEntry(key, newScore, 6, ply, TtHashExact); // Deeper depth

    auto retrieved = readHashEntry(key, alpha, beta, 4, ply);
    REQUIRE(retrieved.has_value());
    REQUIRE(retrieved.value() == newScore);
}

TEST_CASE("Transposition Table - Collision Handling", "[TT]")
{
    clearTable();

    uint64_t key1 = 0x1111111111111111;
    uint64_t key2 = key1 + s_ttHashSize; // Forces collision

    writeHashEntry(key1, 30, depth, ply, TtHashExact);
    writeHashEntry(key2, 99, depth, ply, TtHashExact);

    auto retrieved1 = readHashEntry(key1, alpha, beta, depth, ply);
    auto retrieved2 = readHashEntry(key2, alpha, beta, depth, ply);

    REQUIRE_FALSE(retrieved1.has_value());

    REQUIRE(retrieved2.has_value());
    REQUIRE(retrieved2.value() == 99);
}

TEST_CASE("Transposition Table - Score Clamping for Mating Scores", "[TT]")
{
    clearTable();

    constexpr int16_t nearMate = s_mateValue - 10;

    uint64_t key = 0xCAFEBABEDEADBEEF;

    writeHashEntry(key, nearMate, depth, ply, TtHashExact);

    SECTION("From same ply")
    {
        auto retrieved = readHashEntry(key, alpha, beta, depth, ply);

        REQUIRE(retrieved.has_value());
        REQUIRE(retrieved.value() == nearMate); // same score
    }

    SECTION("From higher ply")
    {
        uint8_t plyOffset = 5;
        auto retrieved = readHashEntry(key, alpha, beta, depth, ply + plyOffset);

        REQUIRE(retrieved.has_value());
        REQUIRE(retrieved.value() == nearMate - plyOffset); // lower score
    }

    SECTION("From lower ply")
    {
        uint8_t plyOffset = 1;
        auto retrieved = readHashEntry(key, alpha, beta, depth, ply - plyOffset);

        REQUIRE(retrieved.has_value());
        REQUIRE(retrieved.value() == nearMate + plyOffset); // higher score
    }
}

TEST_CASE("Transposition Table - Clear Functionality", "[TT]")
{
    clearTable();

    uint64_t key = 0xF00DBABE12345678;
    writeHashEntry(key, 77, depth, ply, TtHashExact);

    clearTable();

    auto retrieved = readHashEntry(key, alpha, beta, depth, ply);
    REQUIRE_FALSE(retrieved.has_value()); // Should be cleared
}

