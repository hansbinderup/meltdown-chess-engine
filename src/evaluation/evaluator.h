#pragma once

#include "magic_enum/magic_enum.hpp"
#include "src/bit_board.h"
#include "src/movement/move_types.h"
#include <iostream>

namespace evaluation {

class Evaluator {
public:
    Evaluator(const Evaluator&) = delete;
    Evaluator(Evaluator&&) = delete;
    Evaluator& operator=(const Evaluator&) = delete;
    Evaluator& operator=(Evaluator&&) = delete;

    Evaluator(FileLogger& m_logger)
        : m_logger(m_logger)
    {
    }

    constexpr movement::Move getBestMove(const BitBoard& board, std::optional<uint8_t> depthInput = std::nullopt)
    {
        m_logger.log("Get best move for {}", magic_enum::enum_name(board.getCurrentPlayer()));

        const uint8_t depth = depthInput.value_or(5);
        return scanForBestMove(depth, board);
    }

private:
    constexpr void reset(uint8_t depth)
    {
        m_bestMove = std::nullopt;
        m_nodes = 0;
        m_depth = depth;
    }

    constexpr movement::Move scanForBestMove(uint8_t depth, const BitBoard& board)
    {
        reset(depth);

        int16_t score = negamax(depth, board, s_minScore, s_maxScore);
        std::cout << std::format("info score cp {} depth {} nodes {}\n", score, depth, m_nodes);

        if (m_bestMove.has_value()) {
            m_logger.log("Found best move: {}, score={}, nodes: {}", movement::moveToString(m_bestMove.value()), score, m_nodes);
            return m_bestMove.value();
        }

        m_logger.log("No move was found, aborting");
        std::cerr << "No move was found, aborting" << std::endl;
        std::exit(1);
    }

    constexpr int16_t negamax(uint8_t depth, const BitBoard& board, int16_t alpha = s_minScore, int16_t beta = s_maxScore)
    {
        if (depth == 0) {
            return quiesence(board, alpha, beta);
        }

        m_nodes++;

        std::optional<movement::Move> currentBestMove;
        int16_t prevAlpha = alpha;
        uint16_t legalMoves = 0;

        const Player currentPlayer = board.getCurrentPlayer();
        const uint8_t currentDepth = m_depth - depth;

        auto allMoves = board.getAllMoves();
        for (const auto& move : allMoves.getMoves()) {
            BitBoard newBoard = board;
            newBoard.performMove(move);

            if (newBoard.isKingAttacked(currentPlayer)) {
                // invalid move
                continue;
            }

            legalMoves++;
            const int16_t score = -negamax(depth - 1, newBoard, -beta, -alpha);

            if (score >= beta)
                // change to beta for hard cutoff
                return beta;

            if (score > alpha) {
                alpha = score;

                // root move -> we can add it as a current best move
                if (currentDepth == 0) {
                    currentBestMove = move;
                }
            }
        }

        if (legalMoves == 0) {
            if (board.isKingAttacked()) {
                // We want absolute negative score - but with amount of moves to the given checkmate
                // we add the ply to make checkmate in less moves a better move
                return s_minScore + currentDepth;
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

    constexpr int16_t quiesence(const BitBoard& board, int16_t alpha, int16_t beta)
    {
        m_nodes++;

        const Player currentPlayer = board.getCurrentPlayer();
        const int16_t evaluation = board.getScore();

        // Hard cutoff
        if (evaluation >= beta) {
            return beta;
        }

        if (evaluation > alpha) {
            alpha = evaluation;
        }

        auto allMoves = board.getAllCaptures();
        for (const auto& move : allMoves.getMoves()) {
            BitBoard newBoard = board;
            newBoard.performMove(move);

            if (newBoard.isKingAttacked(currentPlayer)) {
                // invalid move
                continue;
            }

            const int16_t score = -quiesence(newBoard, -beta, -alpha);
            if (score >= beta)
                // change to beta for hard cutoff
                return beta;

            if (score > alpha) {
                alpha = score;
            }
        }

        return alpha;
    }

    FileLogger& m_logger;
    std::optional<movement::Move> m_bestMove;
    uint64_t m_nodes {};
    uint8_t m_depth;
};
}
