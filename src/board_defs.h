#pragma once

#include <cstdint>

enum class Player : bool {
    White = true,
    Black = false,
};

constexpr Player nextPlayer(Player p)
{
    return static_cast<Player>(!static_cast<bool>(p));
}

constexpr static inline uint8_t s_firstRow { 0 };
constexpr static inline uint8_t s_secondRow { 8 };
constexpr static inline uint8_t s_seventhRow { 48 };
constexpr static inline uint8_t s_eightRow { 56 };

constexpr static inline uint16_t s_maxMoves { 256 };
constexpr static inline uint64_t s_aFileMask { 0x0101010101010101 };
constexpr static inline uint64_t s_hFileMask { 0x8080808080808080 };

constexpr static inline uint64_t s_row2Mask { 0xffULL << s_secondRow };
constexpr static inline uint64_t s_row7Mask { 0xffULL << s_seventhRow };
