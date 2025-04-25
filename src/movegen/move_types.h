#pragma once

#include "board_defs.h"
#include "helpers/bit_operations.h"

#include <cstdint>

namespace movegen {

enum MoveType {
    MovePseudoLegal,
    MoveCapture,
};

/*
 * Moves are encoded as:
 * 0000 0000 0000 0000 0000 0000 0011 1111 -> from value
 * 0000 0000 0000 0000 0000 1111 1100 0000 -> to value
 * 0000 0000 0000 0000 1111 0000 0000 0000 -> piece type
 * 0000 0000 0000 0111 0000 0000 0000 0000 -> promotion type
 * 0000 0000 0111 1000 0000 0000 0000 0000 -> castle type
 * 0000 0000 1000 0000 0000 0000 0000 0000 -> is capture
 * 0000 0001 0000 0000 0000 0000 0000 0000 -> enables en pessant
 * 0000 0010 0000 0000 0000 0000 0000 0000 -> takes en pessant
 * */
class Move {
public:
    // allow default construction for resetting / initializing arrays
    Move() = default;

    explicit Move(uint32_t ext_data)
        : data(ext_data) { };

    explicit Move(uint64_t from, uint64_t to, uint64_t piece, uint64_t promotion, uint64_t castle, uint64_t capture, uint64_t enPessant, uint64_t takeEnPessant)
        : data(
              from
              | to << s_toShift
              | piece << s_pieceShift
              | promotion << s_promotionShift
              | castle << s_castlingShift
              | capture << s_captureShift
              | enPessant << s_enPessantShift
              | takeEnPessant << s_takeEnPessantShift)
    {
    }

    /* helper to create a simple move */
    constexpr static inline Move create(uint8_t from, uint8_t to, Piece piece, bool capture)
    {
        return Move(from, to, piece, PromotionNone, CastleNone, capture, false, false);
    }

    /* helper to create a promotion move */
    constexpr static inline Move createPromotion(uint8_t from, uint8_t to, Piece piece, PromotionType promotion, bool capture)
    {
        return Move(from, to, piece, promotion, CastleNone, capture, false, false);
    }

    /* helper to create a castle move */
    constexpr static inline Move createCastle(uint8_t from, uint8_t to, Piece piece, CastleType castle)
    {
        return Move(from, to, piece, PromotionNone, castle, false, false, false);
    }

    constexpr static inline Move createEnPessant(uint8_t from, uint8_t to, Piece piece, bool enPessant, bool takeEnPessant)
    {
        return Move(from, to, piece, PromotionNone, CastleNone, takeEnPessant, enPessant, takeEnPessant);
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

    constexpr inline Piece piece() const
    {
        return static_cast<Piece>((data >> s_pieceShift) & s_pieceMask);
    }

    constexpr inline PromotionType promotionType() const
    {
        return static_cast<PromotionType>((data >> s_promotionShift) & s_promotionMask);
    }

    constexpr inline bool isPromotionMove() const
    {
        return promotionType() != PromotionNone;
    }

    constexpr inline CastleType castleType() const
    {
        return static_cast<CastleType>((data >> s_castlingShift) & s_castlingMask);
    }

    constexpr inline bool isCastleMove() const
    {
        return castleType() != CastleNone;
    }

    constexpr inline bool isCapture() const
    {
        constexpr uint64_t captureMask = 1ULL << s_captureShift;
        return data & captureMask;
    }

    constexpr inline bool hasEnPessant() const
    {
        constexpr uint64_t enPessantMask = 1ULL << s_enPessantShift;
        return data & enPessantMask;
    }

    constexpr inline bool takeEnPessant() const
    {
        constexpr uint64_t takeEnPessantMask = 1ULL << s_takeEnPessantShift;
        return data & takeEnPessantMask;
    }

    constexpr inline bool isNull() const
    {
        return data == 0;
    }

    constexpr inline std::array<char, 6> toString() const
    {
        std::array<char, 6> buffer {};

        buffer[0] = 'a' + (fromPos() % 8); // Column
        buffer[1] = '1' + (fromPos() / 8); // Row
        buffer[2] = 'a' + (toPos() % 8); // Column
        buffer[3] = '1' + (toPos() / 8); // Row

        const auto p = promotionType();
        if (p != PromotionNone) {
            buffer[4] = promotionToString(p);
            buffer[5] = '\0';
        } else {
            buffer[4] = '\0';
        }

        return buffer;
    }

private:
    uint32_t data;

    constexpr static inline uint64_t s_toFromMask { 0b111111 }; /* 64 values */
    constexpr static inline uint64_t s_toShift { 6 };

    constexpr static inline uint64_t s_pieceMask { 0b1111 }; /* 16 values (only 12 used) */
    constexpr static inline uint64_t s_pieceShift { 12 };

    constexpr static inline uint64_t s_promotionMask { 0b111 }; /* 8 values (only 5 used) */
    constexpr static inline uint64_t s_promotionShift { 16 };

    constexpr static inline uint64_t s_castlingMask { 0b1111 }; /* 4 bit flags */
    constexpr static inline uint64_t s_castlingShift { 19 };

    // Bool so no need for mask
    constexpr static inline uint64_t s_captureShift { 23 };
    constexpr static inline uint64_t s_enPessantShift { 24 };
    constexpr static inline uint64_t s_takeEnPessantShift { 25 };
};

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
