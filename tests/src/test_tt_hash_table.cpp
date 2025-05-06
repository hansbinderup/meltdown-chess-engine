
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>

#define private public
#include <engine/tt_hash_table.h>

using namespace engine;

constexpr uint8_t depth = 5;
constexpr uint8_t ply = 2;

TEST_CASE("Transposition Table - Basic Write & Read", "[TT]")
{
    engine::TtHashTable::setSizeMb(16);

    uint64_t key = 0x123456789ABCDEF;
    Score score = 42;

    const auto move = movegen::Move::create(A2, A4, false);

    TtHashTable::writeEntry(key, score, move, depth, ply, TtHashExact);
    auto retrieved = TtHashTable::TtHashTable::probe(key, depth, ply);

    REQUIRE(retrieved.has_value());
    REQUIRE(retrieved->score == score);
    REQUIRE(retrieved->move == move);
    REQUIRE(retrieved->flag == TtHashExact);
}

TEST_CASE("Transposition Table - Edge Case for Positive Score", "[TT]")
{
    engine::TtHashTable::setSizeMb(16);

    uint64_t key = 0x123456789ABCDEF;
    Score score = s_maxScore;

    const auto move = movegen::Move::create(A2, A4, false);

    TtHashTable::writeEntry(key, score, move, depth, ply, TtHashExact);
    auto retrieved = TtHashTable::TtHashTable::probe(key, depth, ply);

    REQUIRE(retrieved.has_value());
    REQUIRE(retrieved->score == score);
    REQUIRE(retrieved->move == move);
    REQUIRE(retrieved->flag == TtHashExact);
}

TEST_CASE("Transposition Table - Edge Case for Negative Score", "[TT]")
{
    engine::TtHashTable::setSizeMb(16);

    uint64_t key = 0x123456789ABCDEF;
    Score score = s_minScore;

    const auto move = movegen::Move::create(A2, A4, false);

    TtHashTable::writeEntry(key, score, move, depth, ply, TtHashExact);
    auto retrieved = TtHashTable::TtHashTable::probe(key, depth, ply);

    REQUIRE(retrieved.has_value());
    REQUIRE(retrieved->score == score);
    REQUIRE(retrieved->move == move);
    REQUIRE(retrieved->flag == TtHashExact);
}

TEST_CASE("Transposition Table - Depth Overwrite Rule", "[TT]")
{
    engine::TtHashTable::setSizeMb(16);

    uint64_t key = 0xDEADBEEFCAFEBABE;
    Score oldScore = 10;
    Score newScore = 50;

    const auto move1 = movegen::Move::create(A2, A4, false);
    const auto move2 = movegen::Move::create(D7, D7, false);

    TtHashTable::writeEntry(key, oldScore, move1, 3, ply, TtHashExact);
    TtHashTable::writeEntry(key, newScore, move2, 6, ply, TtHashExact); // Deeper depth

    auto retrieved = TtHashTable::probe(key, 4, ply);
    REQUIRE(retrieved.has_value());
    REQUIRE(retrieved->score == newScore);
    REQUIRE(retrieved->move == move2);
    REQUIRE(retrieved->flag == TtHashExact);
}

TEST_CASE("Transposition Table - Collision Handling", "[TT]")
{
    engine::TtHashTable::setSizeMb(16);

    uint64_t key1 = 0x1111111111111111;
    uint64_t key2 = key1 + TtHashTable::s_ttHashSize; // Forces collision

    const auto move1 = movegen::Move::create(A2, A4, false);
    const auto move2 = movegen::Move::create(D7, D7, false);

    TtHashTable::writeEntry(key1, 30, move1, depth, ply, TtHashExact);
    TtHashTable::writeEntry(key2, 99, move2, depth, ply, TtHashExact);

    auto retrieved1 = TtHashTable::probe(key1, depth, ply);
    auto retrieved2 = TtHashTable::probe(key2, depth, ply);

    REQUIRE_FALSE(retrieved1.has_value());

    REQUIRE(retrieved2.has_value());
    REQUIRE(retrieved2->score == 99);
    REQUIRE(retrieved2->move == move2);
    REQUIRE(retrieved2->flag == TtHashExact);
}

TEST_CASE("Transposition Table - Score Clamping for Mating Scores", "[TT]")
{
    engine::TtHashTable::setSizeMb(16);

    constexpr Score nearMate = s_mateValue - 10;

    uint64_t key = 0xCAFEBABEDEADBEEF;

    const auto move = movegen::Move::create(A2, A4, false);
    TtHashTable::writeEntry(key, nearMate, move, depth, ply, TtHashExact);

    SECTION("From same ply")
    {
        auto retrieved = TtHashTable::probe(key, depth, ply);

        REQUIRE(retrieved.has_value());
        REQUIRE(retrieved->score == nearMate); // same score
        REQUIRE(retrieved->move == move);
        REQUIRE(retrieved->flag == TtHashExact);
    }

    SECTION("From higher ply")
    {
        uint8_t plyOffset = 5;
        auto retrieved = TtHashTable::probe(key, depth, ply + plyOffset);

        REQUIRE(retrieved.has_value());
        REQUIRE(retrieved->score == nearMate - plyOffset); // lower score
        REQUIRE(retrieved->move == move);
        REQUIRE(retrieved->flag == TtHashExact);
    }

    SECTION("From lower ply")
    {
        uint8_t plyOffset = 1;

        auto retrieved = TtHashTable::probe(key, depth, ply - plyOffset);

        REQUIRE(retrieved.has_value());
        REQUIRE(retrieved->score == nearMate + plyOffset); // higher score
        REQUIRE(retrieved->move == move);
        REQUIRE(retrieved->flag == TtHashExact);
    }
}

TEST_CASE("Transposition Table - Clear Functionality", "[TT]")
{
    engine::TtHashTable::setSizeMb(16);

    uint64_t key = 0xF00DBABE12345678;
    const auto move = movegen::Move::create(A2, A4, false);
    TtHashTable::writeEntry(key, 77, move, depth, ply, TtHashExact);

    TtHashTable::clear();

    auto retrieved = TtHashTable::probe(key, depth, ply);
    REQUIRE_FALSE(retrieved.has_value()); // Should be cleared
}
