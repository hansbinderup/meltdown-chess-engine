#pragma once

#include "magic_enum/magic_enum.hpp"
#include "src/engine/move_handling.h"
#include "src/evaluation/material_scoring.h"
#include "src/evaluation/pv_table.h"
#include "src/file_logger.h"
#include "src/movement/move_types.h"
#include <chrono>
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
        m_logger.log("Get best move for {}", magic_enum::enum_name(board.player));

        const uint8_t depth = std::clamp(board.getRoundsCount(), s_minDepth, s_maxDepth);
        return scanForBestMove(depthInput.value_or(depth), board);
    }

    constexpr void printEvaluation(const BitBoard& board, std::optional<uint8_t> depthInput = std::nullopt)
    {
        uint8_t depth = depthInput.value_or(3);
        const auto allMoves = engine::getAllMovesSorted(board, 0);

        m_logger << std::format(" Move evaluations [{}]:\n", depth);
        for (const auto& move : allMoves.getMoves()) {
            const auto newBoard = engine::performMove(board, move);

            int16_t score = -negamax(depth - 1, newBoard);

            m_logger << std::format("  move: {}\tscore: {}\tnodes: {} \t pv: ", move.toString().data(), score, m_nodes);
            for (const auto& move : m_pvTable.getMoves()) {
                m_logger << move.toString().data() << " ";
            }
            m_logger << "\n";

            // flush so we can follow each line appear - can take some time
            m_logger.flush();
        }
    }

private:
    void reset()
    {
        m_nodes = 0;
        m_pvTable.reset();
        evaluation::MoveScoring::resetHeuristics();
    }

    constexpr movement::Move scanForBestMove(uint8_t depth, const BitBoard& board)
    {
        using namespace std::chrono;

        reset();

        const auto startTime = system_clock::now();

        /* iterative deeping - should be faster and better? */
        for (uint8_t d = 1; d <= depth; d++) {
            int16_t score = negamax(d, board, s_minScore, s_maxScore);

            const auto endTime = system_clock::now();
            const auto timeDiff = duration_cast<milliseconds>(endTime - startTime).count();

            std::cout << std::format("info score cp {} time {} depth {} seldepth {} nodes {} pv ", score, timeDiff, d, depth, m_nodes);
            for (const auto& move : m_pvTable.getMoves()) {
                std::cout << move.toString().data() << " ";
            }
            std::cout << std::endl;
        }

        return m_pvTable.bestMove();
    }

    constexpr int16_t negamax(uint8_t depth, const BitBoard& board, int16_t alpha = s_minScore, int16_t beta = s_maxScore)
    {
        m_pvTable.updateLength(m_ply);

        if (depth == 0) {
            return quiesence(board, alpha, beta);
        }

        m_nodes++;

        uint16_t legalMoves = 0;
        const Player currentPlayer = board.player;
        const bool isChecked = engine::isKingAttacked(board);

        // Dangerous position - increase search depth
        if (isChecked) {
            depth++;
        }

        auto allMoves = engine::getAllMovesSorted(board, m_ply);
        for (const auto& move : allMoves.getMoves()) {
            if (m_ply == 0) {
                std::cout << "info currmove " << move.toString().data() << " currmovenumber 1" << " nodes " << m_nodes << std::endl;
            }

            const auto newBoard = engine::performMove(board, move);
            if (engine::isKingAttacked(newBoard, currentPlayer)) {
                // invalid move
                continue;
            }

            m_ply++;
            legalMoves++;
            const int16_t score = -negamax(depth - 1, newBoard, -beta, -alpha);
            m_ply--;

            if (score >= beta) {
                evaluation::MoveScoring::updateKillerMove(move, m_ply);
                return beta;
            }

            if (score > alpha) {
                alpha = score;

                evaluation::MoveScoring::updateHistoryMove(board, move, m_ply);
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

    constexpr int16_t quiesence(const BitBoard& board, int16_t alpha, int16_t beta)
    {
        m_nodes++;

        // Engine is not designed to search deeper than this! Make sure to stop before it's too late
        if (m_ply >= s_maxSearchDepth) {
            return beta;
        }

        const int16_t evaluation = evaluation::materialScore(board);

        // Hard cutoff
        if (evaluation >= beta) {
            return beta;
        }

        if (evaluation > alpha) {
            alpha = evaluation;
        }

        auto allMoves = engine::getAllCapturesSorted(board, m_ply);
        for (const auto& move : allMoves.getMoves()) {
            const auto newBoard = engine::performMove(board, move);

            if (engine::isKingAttacked(newBoard, board.player)) {
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

    constexpr static inline uint16_t s_minDepth { 7 };
    constexpr static inline uint16_t s_maxDepth { 7 };
};
}
