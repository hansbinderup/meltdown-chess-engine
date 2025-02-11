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

enum Piece : uint8_t {
    Pawn = 0,
    Knight,
    Bishop,
    Rook,
    Queen,
    King,
};

constexpr static inline uint8_t s_maxSearchDepth { 64 };

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

constexpr static inline uint64_t s_whiteQueenSideCastleMask { 0x11ULL }; // 0b10001 - mask required for king | rook
constexpr static inline uint64_t s_whiteKingSideCastleMask { 0x90ULL }; // 0b10010000 - mask required for king | rook
constexpr static inline uint64_t s_whiteQueenSideCastleAttackMask { 0x1cULL }; // 0b11100 - fields that can block castling if attacked
constexpr static inline uint64_t s_whiteKingSideCastleAttackMask { 0x30ULL }; // 0b110000 - fields that can block castling if attacked
constexpr static inline uint64_t s_whiteQueenSideCastleOccupationMask { 0xeULL }; // 0b1110 - fields that can block castling if occupied
constexpr static inline uint64_t s_whiteKingSideCastleOccupationMask { 0x60ULL }; // 0b1100000 - fields that can block castling if occupied

constexpr static inline uint64_t s_blackQueenSideCastleMask { 0x11ULL << s_eightRow };
constexpr static inline uint64_t s_blackKingSideCastleMask { 0x90ULL << s_eightRow };
constexpr static inline uint64_t s_blackQueenSideCastleAttackMask { 0x1cULL << s_eightRow };
constexpr static inline uint64_t s_blackKingSideCastleAttackMask { 0x30ULL << s_eightRow };
constexpr static inline uint64_t s_blackQueenSideCastleOccupationMask { 0xeULL << s_eightRow };
constexpr static inline uint64_t s_blackKingSideCastleOccupationMask { 0x60ULL << s_eightRow };

