#pragma once

#include "fmt/format.h"
#include <cstdint>

namespace helper {

constexpr void printBitboard(uint64_t board)
{
    std::string output;

    for (uint8_t row = 8; row > 0; row--) {
        output += fmt::format("-{}- ", row);

        for (uint8_t column = 0; column < 8; column++) {
            uint64_t square = 1ULL << (((row - 1) * 8) + column);
            output += (square & board) ? "1 " : "0 ";
        }

        output += '\n';
    }

    output += "    A B C D E F G H\n\n";
    fmt::print("{}", output);
}

}

