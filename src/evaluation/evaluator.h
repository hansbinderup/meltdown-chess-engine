#pragma once

#include "magic_enum/magic_enum.hpp"
#include "src/engine.h"
#include "src/evaluation/pv_table.h"
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

    constexpr movement::Move getBestMove(const Engine& board, std::optional<uint8_t> depthInput = std::nullopt)
    {
        m_logger.log("Get best move for {}", magic_enum::enum_name(board.getCurrentPlayer()));

        const uint8_t depth = std::clamp(board.getRoundsCount(), s_minDepth, s_maxDepth);
        return scanForBestMove(depthInput.value_or(depth), board);
    }

    constexpr void printEvaluation(const Engine& board, std::optional<uint8_t> depthInput = std::nullopt)
    {
        uint8_t depth = depthInput.value_or(s_maxDepth);
        const auto allMoves = board.getAllMovesSorted();

        m_logger << std::format(" Move evaluations [{}]:\n", depth);
        for (const auto& move : allMoves.getMoves()) {
            Engine newBoard = board;
            newBoard.performMove(move);

            int16_t score = -negamax(depth - 1, newBoard);

            m_logger << std::format("  move: {}\tscore: {}\tnodes: {} \t pv: ", move.toString(), score, m_nodes);
            for (const auto& move : m_pvTable.getMoves()) {
                m_logger << move.toString() << " ";
            }
            m_logger << "\n";

            // flush so we can follow each line appear - can take some time
            m_logger.flush();
        }
    }

private:
    constexpr movement::Move scanForBestMove(uint8_t depth, const Engine& board)
    {
        m_nodes = 0;

        int16_t score = negamax(depth, board, s_minScore, s_maxScore);

        std::cout << std::format("info score cp {} depth {} nodes {} pv ", score, depth, m_nodes);
        for (const auto& move : m_pvTable.getMoves()) {
            std::cout << move.toString() << " ";
        }
        std::cout << std::endl;

        return m_pvTable.bestMove();
    }

    constexpr int16_t negamax(uint8_t depth, const Engine& board, int16_t alpha = s_minScore, int16_t beta = s_maxScore)
    {
        m_pvTable.updateLength(m_ply);

        if (depth == 0) {
            return quiesence(board, alpha, beta);
        }

        m_nodes++;

        uint16_t legalMoves = 0;
        const Player currentPlayer = board.getCurrentPlayer();
        const bool isChecked = board.isKingAttacked();

        // Dangerous position - increase search depth
        if (isChecked) {
            depth++;
        }

        auto allMoves = board.getAllMovesSorted();
        for (const auto& move : allMoves.getMoves()) {
            if (m_ply == 0) {
                std::cout << "info currmove " << move.toString() << " currmovenumber 1" << " nodes " << m_nodes << std::endl;
            }

            Engine newBoard = board;
            newBoard.performMove(move);

            if (newBoard.isKingAttacked(currentPlayer)) {
                // invalid move
                continue;
            }

            m_ply++;
            legalMoves++;
            const int16_t score = -negamax(depth - 1, newBoard, -beta, -alpha);
            m_ply--;

            if (score >= beta)
                // change to beta for hard cutoff
                return beta;

            if (score > alpha) {
                alpha = score;

                m_pvTable.updateTable(move, m_ply);
            }
        }

        if (legalMoves == 0) {
            if (isChecked) {
                // We want absolute negative score - but with amount of moves to the given checkmate
                // we add the ply to make checkmate in less moves a better move
                return s_minScore + m_ply;
            } else {
                // Stalemate - absolute neutral score
                return 0;
            }
        }

        return alpha;
    }

    constexpr int16_t quiesence(const Engine& board, int16_t alpha, int16_t beta)
    {
        m_nodes++;

        // Engine is not designed to search deeper than this! Make sure to stop before it's too late
        if (m_ply >= s_maxSearchDepth) {
            return beta;
        }

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
            Engine newBoard = board;
            newBoard.performMove(move);

            if (newBoard.isKingAttacked(currentPlayer)) {
                // invalid move
                continue;
            }

            m_ply++;
            const int16_t score = -quiesence(newBoard, -beta, -alpha);
            m_ply--;

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
    heuristic::PVTable m_pvTable;

    uint64_t m_nodes {};
    uint8_t m_ply;

    constexpr static inline uint16_t s_minDepth { 5 };
    constexpr static inline uint16_t s_maxDepth { 7 };
};
}
