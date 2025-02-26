#pragma once

#include <array>
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
    WhitePawn = 0,
    WhiteKnight,
    WhiteBishop,
    WhiteRook,
    WhiteQueen,
    WhiteKing,
    BlackPawn,
    BlackKnight,
    BlackBishop,
    BlackRook,
    BlackQueen,
    BlackKing,
};

constexpr static inline auto s_whitePieces = std::to_array<Piece>({ WhitePawn, WhiteKnight, WhiteBishop, WhiteRook, WhiteQueen, WhiteKing });
constexpr static inline auto s_blackPieces = std::to_array<Piece>({ BlackPawn, BlackKnight, BlackBishop, BlackRook, BlackQueen, BlackKing });

enum PromotionType : uint8_t {
    PromotionNone = 0,
    PromotionQueen,
    PromotionKnight,
    PromotionBishop,
    PromotionRook,
};

/* castle types as flags */
enum CastleType : uint8_t {
    CastleNone = 0,
    CastleWhiteKingSide = 1 << 0,
    CastleWhiteQueenSide = 1 << 1,
    CastleBlackKingSide = 1 << 2,
    CastleBlackQueenSide = 1 << 3,
};

constexpr inline char promotionToString(PromotionType p)
{
    switch (p) {
    case PromotionNone:
        return ' ';
    case PromotionQueen:
        return 'q';
    case PromotionKnight:
        return 'n';
    case PromotionBishop:
        return 'b';
    case PromotionRook:
        return 'r';
    }

    return ' ';
}

constexpr static inline uint8_t s_maxSearchDepth { 64 };
constexpr static inline uint8_t s_amountSquares { 64 };

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

