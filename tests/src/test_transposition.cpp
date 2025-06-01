
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>

#define private public
#include <core/transposition.h>

using namespace core;

constexpr uint8_t depth = 5;
constexpr uint8_t ply = 2;

TEST_CASE("Transposition Table - Basic Write & Read", "[TT]")
{
    core::TranspositionTable::setSizeMb(16);

    uint64_t key = 0x123456789ABCDEF;
    Score score = 42;
    Score eval = 12;

    const auto move = movegen::Move::create(A2, A4, false);

    TranspositionTable::writeEntry(key, score, eval, move, false, depth, ply, TtExact);
    auto retrieved = TranspositionTable::probe(key);

    REQUIRE(retrieved.has_value());
    REQUIRE(retrieved->score == score);
    REQUIRE(retrieved->eval == eval);
    REQUIRE(retrieved->move == move);
    REQUIRE(retrieved->info.flag() == TtExact);
    REQUIRE(retrieved->info.pv() == false);
}

TEST_CASE("Transposition Table - Edge Case for Positive Score", "[TT]")
{
    core::TranspositionTable::setSizeMb(16);

    uint64_t key = 0x123456789ABCDEF;
    Score score = 2000;
    Score eval = 42;

    const auto move = movegen::Move::create(A2, A4, false);

    TranspositionTable::writeEntry(key, score, eval, move, false, depth, ply, TtExact);
    auto retrieved = TranspositionTable::probe(key);

    REQUIRE(retrieved.has_value());
    REQUIRE(retrieved->score == score);
    REQUIRE(retrieved->eval == eval);
    REQUIRE(retrieved->move == move);
    REQUIRE(retrieved->info.flag() == TtExact);
    REQUIRE(retrieved->info.pv() == false);
}

TEST_CASE("Transposition Table - Edge Case for Negative Score", "[TT]")
{
    core::TranspositionTable::setSizeMb(16);

    uint64_t key = 0x123456789ABCDEF;
    Score score = -2000;
    Score eval = -42;

    const auto move = movegen::Move::create(A2, A4, false);

    TranspositionTable::writeEntry(key, score, eval, move, false, depth, ply, TtExact);
    auto retrieved = TranspositionTable::probe(key);

    REQUIRE(retrieved.has_value());
    REQUIRE(retrieved->score == score);
    REQUIRE(retrieved->eval == eval);
    REQUIRE(retrieved->move == move);
    REQUIRE(retrieved->info.flag() == TtExact);
    REQUIRE(retrieved->info.pv() == false);
}

TEST_CASE("Transposition Table - Depth Overwrite Rule", "[TT]")
{
    core::TranspositionTable::setSizeMb(16);

    uint64_t key = 0xDEADBEEFCAFEBABE;
    Score oldScore = 10;
    Score newScore = 50;
    Score oldEval = 0;
    Score newEval = 10;

    const auto move1 = movegen::Move::create(A2, A4, false);
    const auto move2 = movegen::Move::create(D7, D7, false);

    TranspositionTable::writeEntry(key, oldScore, oldEval - 10, move1, false, 3, ply, TtExact);
    TranspositionTable::writeEntry(key, newScore, newEval, move2, false, 6, ply, TtExact); // Deeper depth

    auto retrieved = TranspositionTable::probe(key);
    REQUIRE(retrieved.has_value());
    REQUIRE(retrieved->score == newScore);
    REQUIRE(retrieved->eval == newEval);
    REQUIRE(retrieved->move == move2);
    REQUIRE(retrieved->info.flag() == TtExact);
    REQUIRE(retrieved->info.pv() == false);
}

TEST_CASE("Transposition Table - Depth Overwrite Rule with PV", "[TT]")
{
    core::TranspositionTable::setSizeMb(16);

    uint64_t key = 0xDEADBEEFCAFEBABE;
    Score oldScore = 10;
    Score newScore = 50;
    Score oldEval = 0;
    Score newEval = 10;

    const auto move1 = movegen::Move::create(A2, A4, false);
    const auto move2 = movegen::Move::create(D7, D7, false);

    TranspositionTable::writeEntry(key, oldScore, oldEval - 10, move1, false, 3, ply, TtExact);
    TranspositionTable::writeEntry(key, oldScore, newEval, move2, true, 0, ply, TtExact); /* depth is too low - don't overwrite */
    TranspositionTable::writeEntry(key, newScore, newEval, move2, true, 1, ply, TtExact); /* lower depth but with PV flag */

    auto retrieved = TranspositionTable::probe(key);
    REQUIRE(retrieved.has_value());
    REQUIRE(retrieved->score == newScore);
    REQUIRE(retrieved->eval == newEval);
    REQUIRE(retrieved->move == move2);
    REQUIRE(retrieved->info.flag() == TtExact);
    REQUIRE(retrieved->info.pv() == true);
}

TEST_CASE("Transposition Table - Collision Handling", "[TT]")
{
    core::TranspositionTable::setSizeMb(16);

    uint64_t key1 = 0x1111111111111111;
    uint64_t key2 = key1 + TranspositionTable::s_tableSize; // Forces collision

    const auto move1 = movegen::Move::create(A2, A4, false);
    const auto move2 = movegen::Move::create(D7, D7, false);

    TranspositionTable::writeEntry(key1, 30, 0, move1, false, depth, ply, TtExact);
    TranspositionTable::writeEntry(key2, 99, 0, move2, false, depth, ply, TtExact);

    auto retrieved1 = TranspositionTable::probe(key1);
    auto retrieved2 = TranspositionTable::probe(key2);

    REQUIRE_FALSE(retrieved1.has_value());

    REQUIRE(retrieved2.has_value());
    REQUIRE(retrieved2->score == 99);
    REQUIRE(retrieved2->move == move2);
    REQUIRE(retrieved2->info.flag() == TtExact);
    REQUIRE(retrieved2->info.pv() == false);
}

TEST_CASE("Transposition Table - absolute mating Scores", "[TT]")
{
    core::TranspositionTable::setSizeMb(16);

    constexpr Score nearMate = s_mateValue - 10;

    uint64_t key = 0xCAFEBABEDEADBEEF;

    const auto move = movegen::Move::create(A2, A4, false);
    TranspositionTable::writeEntry(key, nearMate, 0, move, false, depth, ply, TtExact);

    auto retrieved = TranspositionTable::probe(key);

    REQUIRE(retrieved.has_value());
    REQUIRE(retrieved->score == scoreAbsolute(nearMate, ply)); /* score should be absolute */
    REQUIRE(retrieved->move == move);
    REQUIRE(retrieved->info.flag() == TtExact);
    REQUIRE(retrieved->info.pv() == false);
}

TEST_CASE("Transposition Table - Clear Functionality", "[TT]")
{
    core::TranspositionTable::setSizeMb(16);

    uint64_t key = 0xF00DBABE12345678;
    const auto move = movegen::Move::create(A2, A4, false);
    TranspositionTable::writeEntry(key, 77, 0, move, false, depth, ply, TtExact);

    TranspositionTable::clear();

    auto retrieved = TranspositionTable::probe(key);
    REQUIRE_FALSE(retrieved.has_value()); // Should be cleared
}

TEST_CASE("Transposition Table - Write eval only", "[TT]")
{
    core::TranspositionTable::setSizeMb(16);

    uint64_t key = 0xF00DBABE12345678;
    const auto move = movegen::nullMove();
    Score eval = 42;

    TranspositionTable::writeEntry(key, s_noScore, eval, move, false, depth, ply, TtAlpha);

    auto retrieved = TranspositionTable::probe(key);

    REQUIRE(retrieved.has_value());
    REQUIRE(retrieved->score == s_noScore);
    REQUIRE(retrieved->eval == eval);
    REQUIRE(retrieved->move == move);
    REQUIRE(retrieved->info.flag() == TtAlpha);
    REQUIRE(retrieved->info.pv() == false);

    /* testing entry should never succeed! */
    REQUIRE_FALSE(testEntry(*retrieved, ply, 0, s_minScore, s_maxScore));
    REQUIRE_FALSE(testEntry(*retrieved, ply, 0, s_maxScore, s_minScore));
}

TEST_CASE("Transposition Table - Test entry", "[TT]")
{
    core::TranspositionTable::setSizeMb(16);

    constexpr Score nearMate = s_mateValue - 10;

    uint64_t key1 = 0xCAFEBABEDEADBEEF;
    uint64_t key2 = 0xDEADBEEFCAFEBABE;
    uint64_t key3 = 0xBABECAFEBEEFDEAD;

    const auto move = movegen::Move::create(A2, A4, false);
    TranspositionTable::writeEntry(key1, nearMate, 0, move, false, depth, ply, TtExact);
    TranspositionTable::writeEntry(key2, nearMate, 0, move, false, depth, ply, TtAlpha);
    TranspositionTable::writeEntry(key3, nearMate, 0, move, false, depth, ply, TtBeta);

    auto retrieved = TranspositionTable::probe(key1);

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
        auto retrieved1 = TranspositionTable::probe(key1);
        auto retrieved2 = TranspositionTable::probe(key2);
        auto retrieved3 = TranspositionTable::probe(key3);

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
