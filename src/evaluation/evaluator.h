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

        const uint8_t depth = std::clamp(board.getRoundsCount(), s_minDepth, s_maxDepth);
        return scanForBestMove(depthInput.value_or(depth), board);
    }

    constexpr void printEvaluation(const BitBoard& board, std::optional<uint8_t> depthInput = std::nullopt)
    {
        uint8_t depth = depthInput.value_or(s_maxDepth);
        reset(depth);

        const auto allMoves = board.getAllMovesSorted();

        m_logger << std::format(" Move evaluations [{}]:\n", depth);
        for (const auto& move : allMoves.getMoves()) {
            BitBoard newBoard = board;
            newBoard.performMove(move);

            int16_t score = -negamax(depth - 1, newBoard);

            m_logger << std::format("  move: {}\tscore: {}\tnodes: {}\n", move.toString(), score, m_nodes);

            // flush so we can follow each line appear - can take some time
            m_logger.flush();
        }
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
            m_logger.log("Found best move: {}, score={}, nodes: {}", m_bestMove.value().toString(), score, m_nodes);
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
        const bool isChecked = board.isKingAttacked();

        // Dangerous position - increase search depth
        if (isChecked) {
            depth++;
        }

        auto allMoves = board.getAllMovesSorted();
        for (const auto& move : allMoves.getMoves()) {
            if (currentDepth == 0) {
                std::cout << "info currmove " << move.toString() << " currmovenumber 1" << " nodes " << m_nodes << std::endl;
            }

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
            if (isChecked) {
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

        auto allMoves = board.getAllCapturesSorted();
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

    constexpr static inline uint16_t s_minDepth { 5 };
    constexpr static inline uint16_t s_maxDepth { 7 };
};
}
