#pragma once

#include "board_defs.h"
#include "file_logger.h"
#include "move_generation.h"

class BitBoard {
public:
    BitBoard(FileLogger& logger)
        : m_logger(logger)
    {
        reset();
    }

    gen::ValidMoves getValidMoves()
    {
        gen::ValidMoves validMoves;

        if (m_player == Player::White) {
            gen::getWhitePawnMoves(validMoves, m_whitePawns, getWhiteOccupation(), getBlackOccupation());
        }

        return validMoves;
    }

    constexpr void reset()
    {
        m_whitePawns = { 0xffULL << s_secondRow };
        m_whiteRooks = { 0x81ULL };
        m_whiteBishops = { 0x24ULL };
        m_whiteKnights = { 0x42ULL };
        m_whiteQueens = { 0x08ULL };
        m_whiteKing = { 0x10ULL };

        m_blackPawns = { 0xffULL << s_seventhRow };
        m_blackRooks = { 0x81ULL << s_eightRow };
        m_blackBishops = { 0x24ULL << s_eightRow };
        m_blackKnights = { 0x42ULL << s_eightRow };
        m_blackQueens = { 0x08ULL << s_eightRow };
        m_blackKing = { 0x10ULL << s_eightRow };

        m_player = Player::White;
    }

    void perform_move(const gen::Move& move)
    {
        m_logger.log("move: {}->{}", move.from, move.to);

        std::optional<uint64_t*> piece;
        const auto fromSquare = 1ULL << move.from;

        if (fromSquare & m_whitePawns)
            piece = &m_whitePawns;
        else if (fromSquare & m_whiteRooks)
            piece = &m_whiteRooks;
        else if (fromSquare & m_whiteBishops)
            piece = &m_whiteBishops;
        else if (fromSquare & m_whiteKnights)
            piece = &m_whiteKnights;
        else if (fromSquare & m_whiteQueens)
            piece = &m_whiteQueens;
        else if (fromSquare & m_whiteKing)
            piece = &m_whiteKing;
        else if (fromSquare & m_blackPawns)
            piece = &m_blackPawns;
        else if (fromSquare & m_blackRooks)
            piece = &m_blackRooks;
        else if (fromSquare & m_blackBishops)
            piece = &m_blackBishops;
        else if (fromSquare & m_blackKnights)
            piece = &m_blackKnights;
        else if (fromSquare & m_blackQueens)
            piece = &m_blackQueens;
        else if (fromSquare & m_blackKing)
            piece = &m_blackKing;

        if (!piece.has_value()) {
            m_logger.log("could not find a piece at given position..");
            abort();
        }

        **piece &= ~(1ULL << move.from);
        **piece |= (1ULL << move.to);

        m_player = nextPlayer(m_player);
    }

    void print_board_debug()
    {
        m_logger.log("\nBitboard debug:");

        for (uint8_t row = 8; row > 0; row--) {
            m_logger << std::format("-{}- ", row);

            for (uint8_t column = 0; column < 8; column++) {
                uint64_t square = 1ULL << (((row - 1) * 8) + column);

                if (square & m_whitePawns) {
                    m_logger << "WP ";
                } else if (square & m_whiteRooks) {
                    m_logger << "WR ";
                } else if (square & m_whiteBishops) {
                    m_logger << "WB ";
                } else if (square & m_whiteKnights) {
                    m_logger << "WH ";
                } else if (square & m_whiteQueens) {
                    m_logger << "WQ ";
                } else if (square & m_whiteKing) {
                    m_logger << "WK ";
                } else if (square & m_blackPawns) {
                    m_logger << "BP ";
                } else if (square & m_blackRooks) {
                    m_logger << "BR ";
                } else if (square & m_blackBishops) {
                    m_logger << "BB ";
                } else if (square & m_blackKnights) {
                    m_logger << "BH ";
                } else if (square & m_blackQueens) {
                    m_logger << "BQ ";
                } else if (square & m_blackKing) {
                    m_logger << "BK ";
                } else {
                    m_logger << "## ";
                }
            }

            m_logger << "\n";
        }

        m_logger << "    A  B  C  D  E  F  G  H";
        m_logger << "\n";
        m_logger.flush();
    }

private:
    constexpr uint64_t getWhiteOccupation()
    {
        return m_whitePawns | m_whiteRooks | m_whiteBishops | m_whiteKnights | m_whiteQueens | m_whiteKing;
    }

    constexpr uint64_t getBlackOccupation()
    {
        return m_blackPawns | m_blackRooks | m_blackBishops | m_blackKnights | m_blackQueens | m_blackKing;
    }

    constexpr uint64_t getAllOccupation()
    {
        return getWhiteOccupation() | getBlackOccupation();
    }

    uint64_t m_whitePawns;
    uint64_t m_whiteRooks;
    uint64_t m_whiteBishops;
    uint64_t m_whiteKnights;
    uint64_t m_whiteQueens;
    uint64_t m_whiteKing;

    uint64_t m_blackPawns;
    uint64_t m_blackRooks;
    uint64_t m_blackBishops;
    uint64_t m_blackKnights;
    uint64_t m_blackQueens;
    uint64_t m_blackKing;

    Player m_player { Player::White };

    FileLogger& m_logger;
};
