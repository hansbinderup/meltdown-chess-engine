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

        auto captures = engine::getAllCaptures(board);
        sortMoves(board, captures, m_ply);
        m_logger << std::format("\n\nCaptures[{}]:\n", captures.count());
        for (const auto& move : captures.getMoves()) {
            m_logger << std::format("{} [{}]  ", move.toString().data(), m_scoring.score(board, move, 0));
        }
        m_logger << "\n\n";

        auto allMoves = engine::getAllMoves(board);
        sortMoves(board, allMoves, m_ply);
        m_logger << std::format("Move evaluations [{}]:\n", depth);
        for (const auto& move : allMoves.getMoves()) {
            const auto newBoard = engine::performMove(board, move);

            int16_t score = -negamax(depth, newBoard);

            m_logger << std::format("  move: {}\tscore: {}\tnodes: {} \t pv: ", move.toString().data(), score, m_nodes);

            const auto pvMoves = m_scoring.pvTable().getMoves();
            for (const auto& move : pvMoves) {
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
        m_scoring.reset();
    }

    constexpr movement::Move scanForBestMove(uint8_t depth, const BitBoard& board)
    {
        using namespace std::chrono;

        reset();

        const auto startTime = system_clock::now();

        /* iterative deeping - should be faster and better? */
        for (uint8_t d = 1; d <= depth; d++) {
            m_scoring.pvTable().setIsFollowing(true);

            int16_t score = negamax(d, board, s_minScore, s_maxScore);

            const auto endTime = system_clock::now();
            const auto timeDiff = duration_cast<milliseconds>(endTime - startTime).count();

            std::cout << std::format("info score cp {} time {} depth {} seldepth {} nodes {} pv ", score, timeDiff, d, depth, m_nodes);
            const auto pvMoves = m_scoring.pvTable().getMoves();
            for (const auto& move : pvMoves) {
                std::cout << move.toString().data() << " ";
            }
            std::cout << std::endl;
        }

        return m_scoring.pvTable().bestMove();
    }

    constexpr int16_t negamax(uint8_t depth, const BitBoard& board, int16_t alpha = s_minScore, int16_t beta = s_maxScore)
    {
        m_scoring.pvTable().updateLength(m_ply);

        if (depth == 0) {
            return quiesence(board, alpha, beta);
        }

        // Engine is not designed to search deeper than this! Make sure to stop before it's too late
        if (m_ply >= s_maxSearchDepth) {
            return materialScore(board);
        }

        m_nodes++;

        uint16_t legalMoves = 0;
        const Player currentPlayer = board.player;
        const bool isChecked = engine::isKingAttacked(board);
        bool isPvMove = false;

        // Dangerous position - increase search depth
        if (isChecked) {
            depth++;
        }

        /* auto allMoves = engine::getAllMovesSorted(board, m_ply); */
        auto allMoves = engine::getAllMoves(board);

        if (m_scoring.pvTable().isFollowing()) {
            m_scoring.pvTable().updatePvScoring(allMoves, m_ply);
        }

        sortMoves(board, allMoves, m_ply);
        for (const auto& move : allMoves.getMoves()) {

            const auto newBoard = engine::performMove(board, move);
            if (engine::isKingAttacked(newBoard, currentPlayer)) {
                // invalid move
                continue;
            }

            m_ply++;
            legalMoves++;

            using namespace std::chrono;
            const auto startTime = system_clock::now();
            int16_t score = 0;

            if (isPvMove) {
                /*
                 * https://en.wikipedia.org/wiki/Principal_variation_search
                 * search with a null window
                 */
                score = -negamax(depth - 1, newBoard, -alpha - 1, -alpha);

                /* if it failed high, do a full re-search */
                if ((score > alpha) && (score < beta)) {
                    score = -negamax(depth - 1, newBoard, -beta, -alpha);
                }

            } else {
                score = -negamax(depth - 1, newBoard, -beta, -alpha);
            }

            const auto endTime = system_clock::now();
            const auto timeDiff = duration_cast<milliseconds>(endTime - startTime).count();

            if (timeDiff > 500) {
                std::cout << "info currmove " << move.toString().data() << " currmovenumber " << m_ply + 1 << " nodes " << m_nodes << std::endl;
            }

            m_ply--;

            if (score >= beta) {
                m_scoring.killerMoves().update(move, m_ply);
                return beta;
            }

            if (score > alpha) {
                isPvMove = true;
                alpha = score;

                m_scoring.historyMoves().update(board, move, m_ply);
                m_scoring.pvTable().updateTable(move, m_ply);
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

        const int16_t evaluation = materialScore(board);

        // Hard cutoff
        if (evaluation >= beta) {
            return beta;
        }

        if (evaluation > alpha) {
            alpha = evaluation;
        }

        auto allMoves = engine::getAllCaptures(board);
        sortMoves(board, allMoves, m_ply);

        for (const auto& move : allMoves.getMoves()) {

            const auto newBoard = engine::performMove(board, move);

            if (engine::isKingAttacked(newBoard, board.player)) {
                // invalid move
                continue;
            }

            m_ply++;

            using namespace std::chrono;
            const auto startTime = system_clock::now();
            const int16_t score = -quiesence(newBoard, -beta, -alpha);
            const auto endTime = system_clock::now();
            const auto timeDiff = duration_cast<milliseconds>(endTime - startTime).count();
            if (timeDiff > 500) {
                std::cout << "info currmove " << move.toString().data() << " currmovenumber " << m_ply + 1 << " nodes " << m_nodes << std::endl;
            }

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

    constexpr void sortMoves(const BitBoard& board, movement::ValidMoves& moves, uint8_t ply)
    {
        std::sort(moves.getMoves().begin(), moves.getMoves().end(), [&, this](const auto& a, const auto& b) {
            return m_scoring.score(board, a, ply) > m_scoring.score(board, b, ply);
        });
    }

    FileLogger& m_logger;

    uint64_t m_nodes {};
    uint8_t m_ply;

    MoveScoring m_scoring {};

    constexpr static inline uint16_t s_minDepth { 7 };
    constexpr static inline uint16_t s_maxDepth { 8 };
};
}
