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

    movement::Move getBestMove()
    {
        return scanForBestMove(4, *this);
    }

    movement::ValidMoves getValidMoves() const
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

    constexpr int64_t getScore(Player player) const
    {
        uint64_t score = 0;
        const bool isWhite = (player == Player::White);

        // Material scoring
        score += std::popcount(isWhite ? m_whitePawns : m_blackPawns) * 100;
        score += std::popcount(isWhite ? m_whiteKnights : m_blackKnights) * 320;
        score += std::popcount(isWhite ? m_whiteBishops : m_blackBishops) * 330;
        score += std::popcount(isWhite ? m_whiteRooks : m_blackRooks) * 500;
        score += std::popcount(isWhite ? m_whiteQueens : m_blackQueens) * 900;
        score += std::popcount(isWhite ? m_whiteKing : m_blackKing) * 10000;

        // Positional scoring
        score += positioning::calculatePawnScore(player == Player::White ? m_whitePawns : m_blackPawns, player);
        score += positioning::calculateRookScore(player == Player::White ? m_whiteRooks : m_blackRooks, player);
        score += positioning::calculateBishopScore(player == Player::White ? m_whiteBishops : m_blackBishops, player);
        score += positioning::calculateKnightScore(player == Player::White ? m_whiteKnights : m_blackKnights, player);
        score += positioning::calculateQueenScore(player == Player::White ? m_whiteQueens : m_blackQueens, player);
        score += positioning::calculateKingScore(player == Player::White ? m_whiteKing : m_blackKing, player);

        // Mobility: Number of legal moves
        score += getValidMoves().getMoves().size() * 5;

        return score;
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
        m_moveCount = 0;
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
        m_moveCount++;
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
        m_logger << "\n\n";
        m_logger.flush();
    }

    Player getCurrentPlayer() const
    {
        return m_player;
    }

private:
    constexpr uint64_t getAllAttacks(Player player) const
    {
        if (player == Player::White) {
            uint64_t attacks = gen::getWhitePawnAttacks(m_whitePawns);
            attacks |= gen::getKnightAttacks(m_whiteKnights);
            attacks |= gen::getRookAttacks(m_whiteRooks, getWhiteOccupation() | getBlackOccupation());
            attacks |= gen::getBishopAttacks(m_whiteBishops, getWhiteOccupation() | getBlackOccupation());
            attacks |= gen::getQueenAttacks(m_whiteQueens, getWhiteOccupation() | getBlackOccupation());

            return attacks;
        } else {
            uint64_t attacks = gen::getBlackPawnAttacks(m_blackPawns);
            attacks |= gen::getKnightAttacks(m_blackKnights);
            attacks |= gen::getRookAttacks(m_blackRooks, getWhiteOccupation() | getBlackOccupation());
            attacks |= gen::getBishopAttacks(m_blackBishops, getWhiteOccupation() | getBlackOccupation());
            attacks |= gen::getQueenAttacks(m_blackQueens, getWhiteOccupation() | getBlackOccupation());

            return attacks;
        }
    }

    constexpr movement::Move scanForBestMove(uint8_t depth, const BitBoard& board)
    {
        const auto moves = board.getValidMoves();

        int16_t highestScore = std::numeric_limits<int16_t>::min(); // Start with lowest possible score
        std::optional<movement::Move> bestMove;

        const auto player = board.getCurrentPlayer();

        for (const auto& move : moves.getMoves()) {
            BitBoard newBoard = board;
            newBoard.perform_move(move);

            // If making this move puts the current player in check, skip it
            if (newBoard.isKingAttacked(player))
                continue;

            int16_t score = evaluateMove(depth - 1, newBoard, false);

            if (score > highestScore) {
                highestScore = score;
                bestMove = move;
            }
        }

        return bestMove.value(); // Make sure there is at least one valid move
    }

    constexpr int16_t evaluateMove(uint8_t depth, BitBoard board, bool maximizingPlayer)
    {
        if (depth == 0)
            return board.getScore(board.getCurrentPlayer()); // Return static evaluation at depth 0

        const auto moves = board.getValidMoves();

        int16_t bestScore = maximizingPlayer
            ? std::numeric_limits<int16_t>::min()
            : std::numeric_limits<int16_t>::max();

        for (const auto& move : moves.getMoves()) {
            BitBoard newBoard = board;
            newBoard.perform_move(move);

            // If move puts player in check, discard it
            if (newBoard.isKingAttacked(board.getCurrentPlayer()))
                continue;

            int16_t score = evaluateMove(depth - 1, newBoard, !maximizingPlayer);

            if (maximizingPlayer) {
                bestScore = std::max(bestScore, score);
            } else {
                bestScore = std::min(bestScore, score);
            }
        }

        return bestScore;
    }

    constexpr bool isKingAttacked(Player player)
    {
        if (player == Player::White) {
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
    uint16_t m_moveCount { 0 };

    FileLogger& m_logger;
};

