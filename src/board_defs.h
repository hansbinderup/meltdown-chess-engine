#pragma once

#include <array>
#include <cstdint>
#include <limits>

enum Player {
    PlayerWhite,
    PlayerBlack,
};

constexpr Player nextPlayer(Player p)
{
    if (p == PlayerWhite)
        return PlayerBlack;
    else
        return PlayerWhite;
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

// clang-format off
// numerical value of each position on the board
enum BoardPosition : uint8_t {
    A1, B1, C1, D1, E1, F1, G1, H1,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A8, B8, C8, D8, E8, F8, G8, H8,
};
// clang-format on

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

constexpr static inline uint8_t s_maxSearchDepth { 128 };
constexpr static inline uint8_t s_amountSquares { 64 };
constexpr static inline uint16_t s_maxHalfMoves { 1000 };

constexpr static inline uint8_t s_firstRow { 0 };
constexpr static inline uint8_t s_secondRow { 8 };
constexpr static inline uint8_t s_seventhRow { 48 };
constexpr static inline uint8_t s_eightRow { 56 };

constexpr static inline uint32_t s_maxMoves { 256 };
constexpr static inline uint64_t s_aFileMask { 0x0101010101010101 };
constexpr static inline uint64_t s_hFileMask { 0x8080808080808080 };

constexpr static inline uint64_t s_row2Mask { 0xffULL << s_secondRow };
constexpr static inline uint64_t s_row7Mask { 0xffULL << s_seventhRow };

constexpr static inline int32_t s_maxScore = 50000;
constexpr static inline int32_t s_minScore = -s_maxScore; // num limit is +1 higher than max

constexpr static inline int32_t s_mateValue { s_maxScore - 1000 };
constexpr static inline int32_t s_mateScore { s_maxScore - 2000 };

