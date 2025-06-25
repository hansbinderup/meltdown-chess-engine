#include <cstdint>
#include <iostream>
#include <vector>

#include "../src/movegen/bishops.h"
#include "../src/movegen/rooks.h"

constexpr uint64_t generate_magic_candidate()
{
    return (static_cast<uint64_t>(rand()) & 0xFFFF) << 48 | (static_cast<uint64_t>(rand()) & 0xFFFF) << 32 | (static_cast<uint64_t>(rand()) & 0xFFFF) << 16 | (static_cast<uint64_t>(rand()) & 0xFFFF);
}

constexpr bool test_magic(uint64_t magic, int sq, bool is_rook)
{
    const uint64_t mask = is_rook ? movegen::s_rooksTable[sq] : movegen::s_bishopsTable[sq];
    const int bits = __builtin_popcountll(mask);
    const int size = 1 << bits;

    std::vector<uint64_t> used(size, 0);

    for (uint64_t subset = 0;; subset = (subset - mask) & mask) {
        int idx = (subset * magic) >> (64 - bits);
        if (used[idx] && used[idx] != subset) {
            return false;
        }
        used[idx] = subset;
        if (!subset)
            break;
    }
    return true;
}

constexpr uint64_t find_magic(int sq, bool is_rook)
{
    for (int attempts = 0; attempts < 1'000'000; ++attempts) {
        uint64_t magic = generate_magic_candidate() & generate_magic_candidate() & generate_magic_candidate();
        if (test_magic(magic, sq, is_rook)) {
            return magic;
        }
    }
    return 0; // Fallback if no magic found (should be handled in real use)
}

int main()
{
    std::vector<uint64_t> rooksMagic;
    std::vector<uint64_t> bishopsMagic;
    for (int sq = 0; sq < 64; ++sq) {
        rooksMagic.push_back(find_magic(sq, true));
        bishopsMagic.push_back(find_magic(sq, false));
    }

    std::cout << "constexpr auto s_rooksMagic = std::to_array<uint64_t>({\n";
    for (const auto val : rooksMagic) {
        std::cout << std::format("0x{:016X},\n", val);
    }
    std::cout << ")};\n"
              << std::endl;

    std::cout << "constexpr auto s_bishopsMagic = std::to_array<uint64_t>({\n";
    for (const auto val : bishopsMagic) {
        std::cout << std::format("0x{:016X},\n", val);
    }
    std::cout << "});\n"
              << std::endl;

    return 0;
}
