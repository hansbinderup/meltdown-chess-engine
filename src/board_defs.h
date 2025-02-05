#pragma once

#include <cstdint>
#include <limits>

enum class Player {
    White,
    Black,
};

constexpr Player nextPlayer(Player p)
{
    if (p == Player::White)
        return Player::Black;
    else
        return Player::White;
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

constexpr static inline int16_t s_maxScore = std::numeric_limits<int16_t>::max();
constexpr static inline int16_t s_minScore = -s_maxScore; // num limit is +1 higher than max
