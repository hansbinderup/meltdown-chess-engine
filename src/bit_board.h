#pragma once

#include "attack_generation.h"
#include "board_defs.h"
#include "file_logger.h"

#include "movement/move_types.h"
#include "src/movement/move_generation.h"
#include "src/positioning.h"

class BitBoard {
public:
    BitBoard(FileLogger& logger)
        : m_logger(logger)
    {
        reset();
    }

    movement::ValidMoves getAllMoves() const
    {
        movement::ValidMoves validMoves;

        if (m_player == Player::White) {
            uint64_t attacks = getAllAttacks(Player::Black);
            gen::getKingMoves(validMoves, m_whiteKing, getWhiteOccupation(), attacks);
            gen::getPawnMoves(validMoves, m_player, m_whitePawns, getWhiteOccupation(), getBlackOccupation());
            gen::getKnightMoves(validMoves, m_whiteKnights, getWhiteOccupation());
            gen::getRookMoves(validMoves, m_whiteRooks, getWhiteOccupation(), getBlackOccupation());
            gen::getBishopMoves(validMoves, m_whiteBishops, getWhiteOccupation(), getBlackOccupation());
            gen::getQueenMoves(validMoves, m_whiteQueens, getWhiteOccupation(), getBlackOccupation());
        } else {
            uint64_t attacks = getAllAttacks(Player::White);
            gen::getKingMoves(validMoves, m_blackKing, getBlackOccupation(), attacks);
            gen::getPawnMoves(validMoves, m_player, m_blackPawns, getWhiteOccupation(), getBlackOccupation());
            gen::getKnightMoves(validMoves, m_blackKnights, getBlackOccupation());
            gen::getRookMoves(validMoves, m_blackRooks, getBlackOccupation(), getWhiteOccupation());
            gen::getBishopMoves(validMoves, m_blackBishops, getBlackOccupation(), getWhiteOccupation());
            gen::getQueenMoves(validMoves, m_blackQueens, getBlackOccupation(), getWhiteOccupation());
        }

        return validMoves;
    }

    movement::ValidMoves getAllCaptures() const
    {
        movement::ValidMoves captures;
        const auto allMoves = getAllMoves();
        const auto enemyOccupation = m_player == Player::White ? getBlackOccupation() : getWhiteOccupation();

        for (const auto& move : allMoves.getMoves()) {
            const auto toSquare = 1ULL << move.to;
            if (toSquare & enemyOccupation)
                captures.addMove(move);
        }

        return captures;
    }

    /*
     * Score in favour of current player
     */
    constexpr int16_t getScore() const
    {
        int16_t score = 0;

        // Material scoring
        score += std::popcount(m_whitePawns) * 100;
        score += std::popcount(m_whiteKnights) * 300;
        score += std::popcount(m_whiteBishops) * 350;
        score += std::popcount(m_whiteRooks) * 500;
        score += std::popcount(m_whiteQueens) * 1000;
        score += std::popcount(m_whiteKing) * 10000;

        score -= std::popcount(m_blackPawns) * 100;
        score -= std::popcount(m_blackKnights) * 300;
        score -= std::popcount(m_blackBishops) * 350;
        score -= std::popcount(m_blackRooks) * 500;
        score -= std::popcount(m_blackQueens) * 1000;
        score -= std::popcount(m_blackKing) * 10000;

        // Positional scoring
        score += positioning::calculatePawnScore(m_whitePawns, Player::White);
        score += positioning::calculateRookScore(m_whiteRooks, Player::White);
        score += positioning::calculateBishopScore(m_whiteBishops, Player::White);
        score += positioning::calculateKnightScore(m_whiteKnights, Player::White);
        score += positioning::calculateQueenScore(m_whiteQueens, Player::White);
        score += positioning::calculateKingScore(m_whiteKing, Player::White);

        score -= positioning::calculatePawnScore(m_blackPawns, Player::Black);
        score -= positioning::calculateRookScore(m_blackRooks, Player::Black);
        score -= positioning::calculateBishopScore(m_blackBishops, Player::Black);
        score -= positioning::calculateKnightScore(m_blackKnights, Player::Black);
        score -= positioning::calculateQueenScore(m_blackQueens, Player::Black);
        score -= positioning::calculateKingScore(m_blackKing, Player::Black);

        return getCurrentPlayer() == Player::White ? score : -score;
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
        m_roundsCount = 0;
    }

    void performMove(const movement::Move& move)
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
        m_roundsCount++;
    }

    void printBoardDebug() const
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
        m_logger << "\n\n";
        m_logger.flush();
    }

    Player getCurrentPlayer() const
    {
        return m_player;
    }

    constexpr uint64_t getAllAttacks(Player player) const
    {
        if (player == Player::White) {
            uint64_t attacks = gen::getWhitePawnAttacks(m_whitePawns);
            attacks |= gen::getKnightAttacks(m_whiteKnights);
            attacks |= gen::getRookAttacks(m_whiteRooks, getWhiteOccupation() | getBlackOccupation());
            attacks |= gen::getBishopAttacks(m_whiteBishops, getWhiteOccupation() | getBlackOccupation());
            attacks |= gen::getQueenAttacks(m_whiteQueens, getWhiteOccupation() | getBlackOccupation());
            attacks |= gen::getKingAttacks(m_whiteKing);

            return attacks;
        } else {
            uint64_t attacks = gen::getBlackPawnAttacks(m_blackPawns);
            attacks |= gen::getKnightAttacks(m_blackKnights);
            attacks |= gen::getRookAttacks(m_blackRooks, getWhiteOccupation() | getBlackOccupation());
            attacks |= gen::getBishopAttacks(m_blackBishops, getWhiteOccupation() | getBlackOccupation());
            attacks |= gen::getQueenAttacks(m_blackQueens, getWhiteOccupation() | getBlackOccupation());
            attacks |= gen::getKingAttacks(m_blackKing);

            return attacks;
        }
    }

    constexpr bool isKingAttacked(Player player) const
    {
        if (player == Player::White) {
            return m_whiteKing & getAllAttacks(Player::Black);
        } else {
            return m_blackKing & getAllAttacks(Player::White);
        }
    }

    constexpr bool isKingAttacked() const
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

    constexpr uint64_t getWhiteOccupation() const
    {
        return m_whitePawns | m_whiteRooks | m_whiteBishops | m_whiteKnights | m_whiteQueens | m_whiteKing;
    }

    constexpr uint64_t getBlackOccupation() const
    {
        return m_blackPawns | m_blackRooks | m_blackBishops | m_blackKnights | m_blackQueens | m_blackKing;
    }

    constexpr uint64_t getAllOccupation() const
    {
        return getWhiteOccupation() | getBlackOccupation();
    }

    constexpr uint16_t getRoundsCount() const
    {
        return m_roundsCount;
    }

private:
    // Bitboard represensation of each pieces
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

    // which player to perform next move
    Player m_player;

    // amount of rounds that the game has been played
    uint16_t m_roundsCount;

    FileLogger& m_logger;
};

