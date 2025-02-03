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
        m_logger.log("Get best move for {}", m_player == Player::White ? "White" : "Black");

        const uint8_t depth = 5;
        return scanForBestMove(depth, *this);
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
        m_moveCount = 0;
        m_ply = 0;
        m_nodes = 0;
        m_bestMove = std::nullopt;
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
        if (m_player == Player::White)
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
        m_bestMove = std::nullopt;
        m_nodes = 0;
        m_moveCounter = 0;

        int16_t score = negamax(depth, board, s_minScore, s_maxScore);
        std::cout << std::format("info score cp {} depth {} nodes {}\n", score, depth, m_nodes);

        if (m_bestMove.has_value()) {
            m_logger.log("Found best move: {}, score={}", movement::moveToString(m_bestMove.value()), score);
            return m_bestMove.value();
        }

        m_logger.log("No move was found, aborting");
        std::cerr << "No move was found, aborting" << std::endl;
        std::exit(1);
    }

    constexpr int16_t negamax(uint8_t depth, BitBoard board, int16_t alpha = -s_minScore, int16_t beta = s_maxScore)
    {
        if (depth == 0) {
            return board.getScore();
        }

        m_nodes++;

        std::optional<movement::Move> currentBestMove;
        int16_t prevAlpha = alpha;
        uint16_t legalMoves = 0;
        const Player currentPlayer = board.getCurrentPlayer();

        auto allMoves = board.getAllMoves();
        for (const auto& move : allMoves.getMoves()) {
            m_ply++;

            BitBoard newBoard = board;
            newBoard.perform_move(move);

            if (newBoard.isKingAttacked(currentPlayer)) {
                // invalid move
                m_ply--;
                continue;
            }

            legalMoves++;
            int16_t score = -negamax(depth - 1, newBoard, -beta, -alpha);
            m_ply--;

            if (score >= beta)
                // change to beta for hard cutoff
                return beta;

            if (score > alpha) {
                alpha = score;

                if (m_ply == 0) {
                    currentBestMove = move;
                }
            }
        }

        if (legalMoves == 0) {
            if (board.isKingAttacked()) {
                // We want absolute negative score - but with amount of moves to the given checkmate
                // we add the ply to make checkmate in less moves a better move
                return s_minScore + m_ply;
            } else {
                // Stalemate - absolute neutral score
                return 0;
            }
        }

        if (prevAlpha != alpha) {
            m_bestMove = currentBestMove;
        }

        return alpha;
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

    Player m_player;
    uint16_t m_moveCount;
    uint64_t m_moveCounter;

    std::optional<movement::Move> m_bestMove;
    int16_t m_ply;
    uint64_t m_nodes;

    FileLogger& m_logger;
};

