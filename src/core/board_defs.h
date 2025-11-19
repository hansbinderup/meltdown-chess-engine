#pragma once

#include "evaluation/score.h"
#include "magic_enum/magic_enum.hpp"
#include <array>
#include <chrono>
#include <cstdint>

enum Player : uint8_t {
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

enum ColorlessPiece : uint8_t {
    Pawn = 0,
    Knight,
    Bishop,
    Rook,
    Queen,
    King
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

template<typename T>
[[nodiscard]] constexpr BoardPosition intToBoardPosition(T val) noexcept
    requires std::integral<T>
{
    return static_cast<BoardPosition>(val);
}

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

constexpr inline std::optional<ColorlessPiece> promotionToColorlessPiece(PromotionType promotion)
{
    switch (promotion) {
    case PromotionNone:
        break;
    case PromotionQueen:
        return Queen;
    case PromotionKnight:
        return Knight;
    case PromotionBishop:
        return Bishop;
    case PromotionRook:
        return Rook;
    }

    return std::nullopt;
}

constexpr static inline uint8_t s_amountPieces = magic_enum::enum_count<Piece>();

enum Phases : uint8_t {
    GamePhaseMg = 0,
    GamePhaseEg,
};

constexpr static inline std::array<uint8_t, s_amountPieces> s_piecePhaseValues {
    0,
    1,
    1,
    2,
    4,
    0, /* white */
    0,
    1,
    1,
    2,
    4,
    0, /* black */
};

constexpr static inline uint8_t s_maxSearchDepth { 128 };
constexpr static inline uint8_t s_amountSquares { 64 };
constexpr static inline uint16_t s_maxHalfMoves { 1000 };

constexpr static inline uint8_t s_firstRow { 0 };
constexpr static inline uint8_t s_secondRow { 8 };
constexpr static inline uint8_t s_thirdRow { 16 };
constexpr static inline uint8_t s_fourthRow { 24 };
constexpr static inline uint8_t s_fifthRow { 32 };
constexpr static inline uint8_t s_sixthRow { 40 };
constexpr static inline uint8_t s_seventhRow { 48 };
constexpr static inline uint8_t s_eightRow { 56 };

constexpr static inline uint32_t s_maxMoves { 256 };
constexpr static inline uint64_t s_aFileMask { 0x0101010101010101 };
constexpr static inline uint64_t s_dFileMask { 0x0808080808080808 };
constexpr static inline uint64_t s_eFileMask { 0x1010101010101010 };
constexpr static inline uint64_t s_hFileMask { 0x8080808080808080 };

constexpr static inline uint64_t s_row1Mask { 0xffULL << s_firstRow };
constexpr static inline uint64_t s_row2Mask { 0xffULL << s_secondRow };
constexpr static inline uint64_t s_row3Mask { 0xffULL << s_thirdRow };
constexpr static inline uint64_t s_row4Mask { 0xffULL << s_fourthRow };
constexpr static inline uint64_t s_row5Mask { 0xffULL << s_fifthRow };
constexpr static inline uint64_t s_row6Mask { 0xffULL << s_sixthRow };
constexpr static inline uint64_t s_row7Mask { 0xffULL << s_seventhRow };
constexpr static inline uint64_t s_row8Mask { 0xffULL << s_eightRow };

constexpr static inline uint64_t s_lightSquares = 0x55aa55aa55aa55aa;
constexpr static inline uint64_t s_darkSquares = 0xaa55aa55aa55aa55;
constexpr static inline uint64_t s_centerSquares = (s_row4Mask | s_row5Mask) & (s_dFileMask | s_eFileMask);

constexpr static inline uint64_t s_whiteOutpostRankMask = s_row4Mask | s_row5Mask | s_row6Mask;
constexpr static inline uint64_t s_blackOutpostRankMask = s_row3Mask | s_row4Mask | s_row5Mask;

constexpr static inline std::size_t s_defaultTtSizeMb { 16 };

constexpr static inline uint8_t s_middleGamePhase { 24 };
constexpr static inline size_t s_maxThreads { 128 };

using TimePoint = std::chrono::time_point<std::chrono::steady_clock>;
constexpr static inline std::chrono::milliseconds s_defaultMoveOverhead { 50 };

constexpr static inline std::string_view s_startPosFen { "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1" };
