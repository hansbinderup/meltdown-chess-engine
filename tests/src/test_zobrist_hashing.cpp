#include <catch2/catch_test_macros.hpp>
#include <engine/zobrist_hashing.h>

uint32_t countOccurenciesInAllTables(uint64_t hash)
{
    uint32_t count {};

    for (const auto& table : engine::s_pieceHashTable) {
        count += std::count_if(
            table.begin(),
            table.end(),
            [hash](uint64_t h) { return h == hash; });
    }

    count += std::count_if(
        engine::s_enpessantHashTable.begin(),
        engine::s_enpessantHashTable.end(),
        [hash](uint64_t h) { return h == hash; });

    count += std::count_if(
        engine::s_castlingHashTable.begin(),
        engine::s_castlingHashTable.end(),
        [hash](uint64_t h) { return h == hash; });

    if (hash == engine::s_playerKey) {
        count++;
    }

    return count;
}

TEST_CASE("Zobrist hashing", "[zobrist]")
{
    SECTION("Test piece hash uniqueness")
    {
        for (const auto& table : engine::s_pieceHashTable) {
            for (const auto hash : table) {
                REQUIRE(countOccurenciesInAllTables(hash) == 1);
            }
        }
    }

    SECTION("Test enpessant hash uniqueness")
    {
        for (const auto hash : engine::s_enpessantHashTable) {
            REQUIRE(countOccurenciesInAllTables(hash) == 1);
        }
    }

    SECTION("Test castling hash uniqueness")
    {
        for (const auto hash : engine::s_castlingHashTable) {
            REQUIRE(countOccurenciesInAllTables(hash) == 1);
        }
    }

    SECTION("Test player hash uniqueness")
    {
        REQUIRE(countOccurenciesInAllTables(engine::s_playerKey) == 1);
    }
}
