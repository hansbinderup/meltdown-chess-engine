#pragma once

#include "src/file_logger.h"

namespace {

constexpr void printBitboard(FileLogger& logger, uint64_t board)
{
    for (uint8_t row = 8; row > 0; row--) {
        logger << std::format("-{}- ", row);

        for (uint8_t column = 0; column < 8; column++) {
            uint64_t square = 1ULL << (((row - 1) * 8) + column);
            if (square & board) {
                logger << "1 ";
            } else {
                logger << "0 ";
            }
        }

        logger << "\n";
    }

    logger << "    A B C D E F G H";
    logger << "\n\n";
    logger.flush();
}

}

