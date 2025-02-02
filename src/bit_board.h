#pragma once

#include "attack_generation.h"
#include "board_defs.h"
#include "file_logger.h"

#include "movement/move_types.h"
#include "src/movement/move_generation.h"

class BitBoard {
public:
    BitBoard(FileLogger& logger)
        : m_logger(logger)
    {
        reset();
    }

    movement::ValidMoves getValidMoves()
    {
        movement::ValidMoves validMoves;

        if (m_player == Player::White) {
            uint64_t attacks = getAllAttacks(Player::Black);

            bool kingAttacked = m_whiteKing & attacks;
            if (kingAttacked) {
                gen::getKingMoves(validMoves, m_whiteKing, getWhiteOccupation(), attacks);
            } else {
                gen::getPawnMoves(validMoves, m_player, m_whitePawns, getWhiteOccupation(), getBlackOccupation());
                gen::getKnightMoves(validMoves, m_whiteKnights, getWhiteOccupation());
                gen::getRookMoves(validMoves, m_whiteRooks, getWhiteOccupation());
                gen::getBishopMoves(validMoves, m_whiteBishops, getWhiteOccupation());
            }
        } else {
            uint64_t attacks = getAllAttacks(Player::White);

            bool kingAttacked = m_blackKing & attacks;
            if (kingAttacked) {
                gen::getKingMoves(validMoves, m_blackKing, getBlackOccupation(), attacks);
            } else {
                gen::getPawnMoves(validMoves, m_player, m_blackPawns, getWhiteOccupation(), getBlackOccupation());
                gen::getKnightMoves(validMoves, m_blackKnights, getBlackOccupation());
                gen::getRookMoves(validMoves, m_blackRooks, getWhiteOccupation());
                gen::getBishopMoves(validMoves, m_blackBishops, getWhiteOccupation());
            }
        }

        return validMoves;
    }

    constexpr void reset()
    {
        // Binary represensation of each piece at start of game
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

    void perform_move(const movement::Move& move)
    {
        const auto fromSquare = 1ULL << move.from;
        const auto toSquare = 1ULL << move.to;

        bitToggleMove(m_whitePawns, fromSquare, toSquare);
        bitToggleMove(m_whiteRooks, fromSquare, toSquare);
        bitToggleMove(m_whiteBishops, fromSquare, toSquare);
        bitToggleMove(m_whiteKnights, fromSquare, toSquare);
        bitToggleMove(m_whiteQueens, fromSquare, toSquare);
        bitToggleMove(m_whiteKing, fromSquare, toSquare);

        bitToggleMove(m_blackPawns, fromSquare, toSquare);
        bitToggleMove(m_blackRooks, fromSquare, toSquare);
        bitToggleMove(m_blackBishops, fromSquare, toSquare);
        bitToggleMove(m_blackKnights, fromSquare, toSquare);
        bitToggleMove(m_blackQueens, fromSquare, toSquare);
        bitToggleMove(m_blackKing, fromSquare, toSquare);

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
    constexpr uint64_t getAllAttacks(Player player)
    {
        if (player == Player::White) {
            uint64_t attacks = gen::getWhitePawnAttacks(m_whitePawns);
            attacks |= gen::getKnightAttacks(m_whiteKnights);

            return attacks;
        } else {
            uint64_t attacks = gen::getBlackPawnAttacks(m_blackPawns);
            attacks |= gen::getKnightAttacks(m_blackKnights);

            return attacks;
        }
    }

    constexpr bool isKingAttacked()
    {
        if (m_player == Player::White) {
            return m_whiteKing & getAllAttacks(Player::Black);
        } else {
            return m_blackKing & getAllAttacks(Player::White);
        }
    }

    constexpr void bitToggleMove(uint64_t& piece, uint64_t fromSquare, uint64_t toSquare)
    {
        if (toSquare & piece) {
            // clear piece if being attacked
            piece &= ~toSquare;
        } else if (fromSquare & piece) {
            // move if given piece is located at "fromSquare"
            piece &= ~fromSquare;
            piece |= toSquare;
        }
    }

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
