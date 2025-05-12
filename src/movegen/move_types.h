#pragma once

#include "board_defs.h"
#include "helpers/bit_operations.h"

#include <cstdint>

namespace movegen {

enum MoveType {
    MovePseudoLegal,
    MoveCapture,
};

/* NOTE: all captures have 0b100 set
 *       all promotions have 0b1000 set */
enum class MoveFlag : uint8_t {
    Quiet = 0b0000,
    DoublePush = 0b0001,
    KingCastle = 0b0010,
    QueenCastle = 0b0011,
    Capture = 0b0100,
    EnPassant = 0b0101,
    KnightPromotion = 0b1000,
    BishopPromotion = 0b1001,
    RookPromotion = 0b1010,
    QueenPromotion = 0b1011,
    KnightPromomotionCapture = 0b1100,
    BishopPromotionCapture = 0b1101,
    RookPromotionCapture = 0b1110,
    QueenPromotionCapture = 0b1111,
};

constexpr MoveFlag promotionFlag(PromotionType promotion, bool capture)
{
    assert(promotion != PromotionNone);

    if (capture) {
        switch (promotion) {
        case PromotionNone:
            break;
        case PromotionQueen:
            return MoveFlag::QueenPromotionCapture;
        case PromotionKnight:
            return MoveFlag::KnightPromomotionCapture;
        case PromotionBishop:
            return MoveFlag::BishopPromotionCapture;
        case PromotionRook:
            return MoveFlag::RookPromotionCapture;
            break;
        }
    }

    switch (promotion) {
    case PromotionNone:
        break;
    case PromotionQueen:
        return MoveFlag::QueenPromotion;
    case PromotionKnight:
        return MoveFlag::KnightPromotion;
    case PromotionBishop:
        return MoveFlag::BishopPromotion;
    case PromotionRook:
        return MoveFlag::RookPromotion;
        break;
    }

    /* should not happen - but satisfy compiler */
    return MoveFlag::Quiet;
}

constexpr MoveFlag castleFlag(CastleType castle)
{
    assert(castle != CastleType::CastleNone);

    switch (castle) {
    case CastleNone:
        break;
    case CastleWhiteKingSide:
    case CastleBlackKingSide:
        return MoveFlag::KingCastle;
    case CastleWhiteQueenSide:
    case CastleBlackQueenSide:
        return MoveFlag::QueenCastle;
        break;
    };

    /* should not happen - but satisfy compiler */
    return MoveFlag::Quiet;
}

/*
 * Moves are encoded as:
 * 0000 0000 0011 1111 -> from value
 * 0000 1111 1100 0000 -> to value
 * 1111 0000 0000 0000 -> move flags
 * */
class Move {
public:
    // allow default construction for resetting / initializing arrays
    Move() = default;

    explicit Move(uint8_t from, uint8_t to, MoveFlag flag)
        : data(
              from
              | to << s_toShift
              | static_cast<uint8_t>(flag) << s_flagShift)
    {
    }

    /* helper to create a simple move */
    constexpr static inline Move create(uint8_t from, uint8_t to, bool capture)
    {
        return Move(from, to, capture ? MoveFlag::Capture : MoveFlag::Quiet);
    }

    /* helper to create a promotion move */
    constexpr static inline Move createPromotion(uint8_t from, uint8_t to, PromotionType promotion, bool capture)
    {
        return Move(from, to, promotionFlag(promotion, capture));
    }

    /* helper to create a castle move */
    constexpr static inline Move createCastle(uint8_t from, uint8_t to, CastleType castle)
    {
        return Move(from, to, castleFlag(castle));
    }

    constexpr static inline Move createEnPessant(uint8_t from, uint8_t to, bool doublePush)
    {
        return Move(from, to, doublePush ? MoveFlag::DoublePush : MoveFlag::EnPassant);
    }

    friend bool operator<=>(const Move& a, const Move& b) = default;

    constexpr inline BoardPosition fromPos() const
    {
        return intToBoardPosition(data & s_toFromMask);
    }

    constexpr inline BoardPosition toPos() const
    {
        return intToBoardPosition((data >> s_toShift) & s_toFromMask);
    }

    constexpr inline uint32_t getData() const
    {
        return data;
    }

    constexpr inline uint64_t fromSquare() const
    {
        return helper::positionToSquare(fromPos());
    }

    constexpr inline uint64_t toSquare() const
    {
        return helper::positionToSquare(toPos());
    }

    constexpr inline PromotionType promotionType() const
    {
        switch (getFlag()) {
        case MoveFlag::KnightPromotion:
        case MoveFlag::KnightPromomotionCapture:
            return PromotionType::PromotionKnight;
        case MoveFlag::BishopPromotion:
        case MoveFlag::BishopPromotionCapture:
            return PromotionType::PromotionBishop;
        case MoveFlag::RookPromotion:
        case MoveFlag::RookPromotionCapture:
            return PromotionType::PromotionRook;
        case MoveFlag::QueenPromotion:
        case MoveFlag::QueenPromotionCapture:
            return PromotionType::PromotionQueen;
        default:
            break;
        }
        return PromotionType::PromotionNone;
    }

    constexpr inline bool isQuietMove() const
    {
        return !isNoisyMove();
    }

    constexpr inline bool isNoisyMove() const
    {
        return isPromotionMove() || isCapture();
    }

    constexpr inline bool isPromotionMove() const
    {
        return (data & (1 << 15)) != 0;
    }

    constexpr inline bool isCastleMove() const
    {
        const auto flag = getFlag();
        return flag == MoveFlag::KingCastle || flag == MoveFlag::QueenCastle;
    }

    constexpr CastleType castleType(Player player) const
    {
        const auto flag = getFlag();
        switch (flag) {
        case MoveFlag::KingCastle:
            return player == PlayerWhite ? CastleWhiteKingSide : CastleBlackKingSide;
        case MoveFlag::QueenCastle:
            return player == PlayerWhite ? CastleWhiteQueenSide : CastleBlackQueenSide;
        default:
            break;
        }

        return CastleType::CastleNone;
    }

    constexpr inline bool isCapture() const
    {
        return (data & (1 << 14)) != 0;
    }

    constexpr inline bool isDoublePush() const
    {
        return getFlag() == MoveFlag::DoublePush;
    }

    constexpr inline bool takeEnPessant() const
    {
        return getFlag() == MoveFlag::EnPassant;
    }

    constexpr inline bool isNull() const
    {
        return data == 0;
    }

private:
    constexpr inline MoveFlag getFlag() const
    {
        return static_cast<MoveFlag>((data >> s_flagShift) & s_flagMask);
    }

    constexpr static inline uint16_t s_toFromMask { 0b111111 }; /* 64 values */
    constexpr static inline uint8_t s_toShift { 6 };

    constexpr static inline uint16_t s_flagMask { 0b1111 }; /* 16 values */
    constexpr static inline uint8_t s_flagShift { 12 };

    uint16_t data = 0;
};

/* helper to create explicit null moves */
constexpr Move nullMove() { return Move(); }

class ValidMoves {
public:
    uint32_t count() const
    {
        return m_count;
    }

    void addMove(Move move)
    {
        m_moves[m_count++] = std::move(move);
    }

    void nullifyMove(uint32_t i)
    {
        m_moves.at(i) = nullMove();
    }

    Move* begin()
    {
        return m_moves.begin();
    }

    Move* end()
    {
        return m_moves.begin() + m_count;
    }

    const Move* begin() const
    {
        return m_moves.begin();
    }

    const Move* end() const
    {
        return m_moves.begin() + m_count;
    }

    Move operator[](int i) const { return m_moves[i]; }
    Move& operator[](int i) { return m_moves[i]; }

private:
    std::array<Move, s_maxMoves> m_moves {};
    std::size_t m_count = 0;
};

}
