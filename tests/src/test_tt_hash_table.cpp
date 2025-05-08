
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
    auto retrieved = TtHashTable::TtHashTable::probe(key);

    REQUIRE(retrieved.has_value());
    REQUIRE(retrieved->score == score);
    REQUIRE(retrieved->move == move);
    REQUIRE(retrieved->flag == TtHashExact);
}

TEST_CASE("Transposition Table - Edge Case for Positive Score", "[TT]")
{
    engine::TtHashTable::setSizeMb(16);

    uint64_t key = 0x123456789ABCDEF;
    Score score = 2000;

    const auto move = movegen::Move::create(A2, A4, false);

    TtHashTable::writeEntry(key, score, move, depth, ply, TtHashExact);
    auto retrieved = TtHashTable::TtHashTable::probe(key);

    REQUIRE(retrieved.has_value());
    REQUIRE(retrieved->score == score);
    REQUIRE(retrieved->move == move);
    REQUIRE(retrieved->flag == TtHashExact);
}

TEST_CASE("Transposition Table - Edge Case for Negative Score", "[TT]")
{
    engine::TtHashTable::setSizeMb(16);

    uint64_t key = 0x123456789ABCDEF;
    Score score = -2000;

    const auto move = movegen::Move::create(A2, A4, false);

    TtHashTable::writeEntry(key, score, move, depth, ply, TtHashExact);
    auto retrieved = TtHashTable::TtHashTable::probe(key);

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

    auto retrieved = TtHashTable::probe(key);
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

    auto retrieved1 = TtHashTable::probe(key1);
    auto retrieved2 = TtHashTable::probe(key2);

    REQUIRE_FALSE(retrieved1.has_value());

    REQUIRE(retrieved2.has_value());
    REQUIRE(retrieved2->score == 99);
    REQUIRE(retrieved2->move == move2);
    REQUIRE(retrieved2->flag == TtHashExact);
}

TEST_CASE("Transposition Table - absolute mating Scores", "[TT]")
{
    engine::TtHashTable::setSizeMb(16);

    constexpr Score nearMate = s_mateValue - 10;

    uint64_t key = 0xCAFEBABEDEADBEEF;

    const auto move = movegen::Move::create(A2, A4, false);
    TtHashTable::writeEntry(key, nearMate, move, depth, ply, TtHashExact);

    auto retrieved = TtHashTable::probe(key);

    REQUIRE(retrieved.has_value());
    REQUIRE(retrieved->score == scoreAbsolute(nearMate, ply)); /* score should be absolute */
    REQUIRE(retrieved->move == move);
    REQUIRE(retrieved->flag == TtHashExact);
}

TEST_CASE("Transposition Table - Clear Functionality", "[TT]")
{
    engine::TtHashTable::setSizeMb(16);

    uint64_t key = 0xF00DBABE12345678;
    const auto move = movegen::Move::create(A2, A4, false);
    TtHashTable::writeEntry(key, 77, move, depth, ply, TtHashExact);

    TtHashTable::clear();

    auto retrieved = TtHashTable::probe(key);
    REQUIRE_FALSE(retrieved.has_value()); // Should be cleared
}

TEST_CASE("Transposition Table - Test entry", "[TT]")
{
    engine::TtHashTable::setSizeMb(16);

    constexpr Score nearMate = s_mateValue - 10;

    uint64_t key1 = 0xCAFEBABEDEADBEEF;
    uint64_t key2 = 0xDEADBEEFCAFEBABE;
    uint64_t key3 = 0xBABECAFEBEEFDEAD;

    const auto move = movegen::Move::create(A2, A4, false);
    TtHashTable::writeEntry(key1, nearMate, move, depth, ply, TtHashExact);
    TtHashTable::writeEntry(key2, nearMate, move, depth, ply, TtHashAlpha);
    TtHashTable::writeEntry(key3, nearMate, move, depth, ply, TtHashBeta);

    auto retrieved = TtHashTable::probe(key1);

    REQUIRE(retrieved.has_value());

    SECTION("Test depth")
    {
        /* tests should only succeed if entry has same or deeper depth */
        REQUIRE_FALSE(testEntry(*retrieved, ply, depth + 1, 0, 0).has_value());
        REQUIRE_FALSE(testEntry(*retrieved, ply, s_maxSearchDepth, 0, 0).has_value());
        REQUIRE(testEntry(*retrieved, ply, depth, 0, 0).has_value());
        REQUIRE(testEntry(*retrieved, ply, depth - 2, 0, 0).has_value());
        REQUIRE(testEntry(*retrieved, ply, 0, 0, 0).has_value());
    }

    SECTION("Test flags")
    {
        auto retrieved1 = TtHashTable::probe(key1);
        auto retrieved2 = TtHashTable::probe(key2);
        auto retrieved3 = TtHashTable::probe(key3);

        REQUIRE(retrieved1.has_value());
        REQUIRE(retrieved2.has_value());
        REQUIRE(retrieved3.has_value());

        /* exact should always give a hit - no matter the alpha and beta */
        REQUIRE(testEntry(*retrieved1, ply, depth, s_maxScore, s_minScore).has_value());
        REQUIRE(testEntry(*retrieved1, ply, depth, s_minScore, s_maxScore).has_value());
        REQUIRE(testEntry(*retrieved1, ply, depth, s_minScore, nearMate).has_value());
        REQUIRE(testEntry(*retrieved1, ply, depth, nearMate, s_maxScore).has_value());

        /* alpha bound should only give a hit if causing a cutoff */
        REQUIRE(testEntry(*retrieved2, ply, depth, s_maxScore, s_minScore).has_value());
        REQUIRE_FALSE(testEntry(*retrieved2, ply, depth, s_minScore, nearMate).has_value());
        REQUIRE(testEntry(*retrieved2, ply, depth, nearMate, s_maxScore).has_value());
        REQUIRE_FALSE(testEntry(*retrieved2, ply, depth, s_minScore, s_maxScore).has_value());

        /* beta bound should only give a hit if causing a cutoff */
        REQUIRE(testEntry(*retrieved3, ply, depth, s_maxScore, s_minScore).has_value());
        REQUIRE(testEntry(*retrieved3, ply, depth, s_minScore, nearMate).has_value());
        REQUIRE_FALSE(testEntry(*retrieved3, ply, depth, nearMate, s_maxScore).has_value());
        REQUIRE_FALSE(testEntry(*retrieved3, ply, depth, s_minScore, s_maxScore).has_value());
    }

    SECTION("Test score")
    {
        const auto test = testEntry(*retrieved, ply, depth, s_maxScore, s_minScore);
        REQUIRE(test.has_value());
        REQUIRE(*test == nearMate); /* score should be converted from absolute -> relative again */
    }
}
