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

    void setWhiteTime(uint64_t time)
    {
        m_whiteTime = time;
    }

    void setBlackTime(uint64_t time)
    {
        m_blackTime = time;
    }

    void setMovesToGo(uint64_t moves)
    {
        m_movesToGo = moves;
    }

    void setMoveTime(uint64_t time)
    {
        m_moveTime = time;
    }

    void setWhiteMoveInc(uint64_t inc)
    {
        m_whiteMoveInc = inc;
    }

    void setBlackMoveInc(uint64_t inc)
    {
        m_blackMoveInc = inc;
    }

    void reset()
    {
        m_nodes = 0;
        m_scoring.reset();

        m_whiteTime = 0;
        m_blackTime = 0;
        m_movesToGo = 0;
        m_moveTime = 0;
        m_whiteMoveInc = 0;
        m_blackMoveInc = 0;
    }

private:
    constexpr movement::Move scanForBestMove(uint8_t depth, const BitBoard& board)
    {
        using namespace std::chrono;
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
        uint64_t movesSearched = 0;
        const Player currentPlayer = board.player;
        const bool isChecked = engine::isKingAttacked(board);

        // Dangerous position - increase search depth
        if (isChecked) {
            depth++;
        }

        /*
         * null move pruning
         * https://www.chessprogramming.org/Null_Move_Pruning
         * */
        if (depth >= 3 && !isChecked && m_ply) {
            auto boardCopy = board;

            /* TODO: add en pessant handling when implemented */

            /* give opponent an extra move */
            boardCopy.player = nextPlayer(boardCopy.player);

            /* perform search with reduced depth (based on reduction limit) */
            int16_t score = -negamax(depth - 1 - s_nullMoveReduction, boardCopy, -beta, -beta + 1);

            if (score >= beta) {
                return beta;
            }
        }

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

            const auto startTime = std::chrono::system_clock::now();
            int16_t score = 0;

            /*
             * LMR
             * https://wiki.sharewiz.net/doku.php?id=chess:programming:late_move_reduction
             */
            if (movesSearched == 0) {
                score = -negamax(depth - 1, newBoard, -beta, -alpha);
            } else {
                if (movesSearched >= s_fullDepthMove
                    && depth >= s_reductionLimit
                    && !isChecked
                    && !move.isCapture()
                    && !move.isPromotionMove()) {
                    /* search current move with reduced depth */
                    score = -negamax(depth - 2, newBoard, -alpha - 1, -alpha);
                } else {
                    /* TODO: hack to ensure full depth is reached */
                    score = alpha + 1;
                }

                /*
                 * PVS
                 * https://en.wikipedia.org/wiki/Principal_variation_search
                 * search with a null window
                 */
                if (score > alpha) {
                    score = -negamax(depth - 1, newBoard, -alpha - 1, -alpha);

                    /* if it failed high, do a full re-search */
                    if ((score > alpha) && (score < beta)) {
                        score = -negamax(depth - 1, newBoard, -beta, -alpha);
                    }
                }
            }

            printMoveInfo(move, startTime);

            m_ply--;
            movesSearched++;

            if (score >= beta) {
                m_scoring.killerMoves().update(move, m_ply);
                return beta;
            }

            if (score > alpha) {
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

    void printMoveInfo(const movement::Move& move, const std::chrono::time_point<std::chrono::system_clock>& startTime)
    {
        using namespace std::chrono;

        const auto endTime = system_clock::now();
        const auto timeDiff = duration_cast<milliseconds>(endTime - startTime).count();

        if (timeDiff > 500) {
            std::cout << "info currmove " << move.toString().data() << " currmovenumber " << m_ply + 1 << " nodes " << m_nodes << std::endl;
        }
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

            const auto startTime = std::chrono::system_clock::now();
            const int16_t score = -quiesence(newBoard, -beta, -alpha);

            printMoveInfo(move, startTime);

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

    uint64_t m_whiteTime {};
    uint64_t m_blackTime {};
    uint64_t m_movesToGo {};
    uint64_t m_moveTime {};
    uint64_t m_whiteMoveInc {};
    uint64_t m_blackMoveInc {};

    std::chrono::time_point<std::chrono::system_clock> m_endTime;

    /* search configs */
    constexpr static inline uint16_t s_minDepth { 7 };
    constexpr static inline uint16_t s_maxDepth { 10 };

    constexpr static inline uint16_t s_fullDepthMove { 4 };
    constexpr static inline uint16_t s_reductionLimit { 3 };

    constexpr static inline uint8_t s_nullMoveReduction { 2 };
};
}
