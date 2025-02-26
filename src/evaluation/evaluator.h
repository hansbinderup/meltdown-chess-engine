#pragma once

#include "src/engine/move_handling.h"
#include "src/engine/tt_hash.h"
#include "src/evaluation/material_scoring.h"
#include "src/evaluation/move_scoring.h"
#include "src/evaluation/pv_table.h"
#include "src/file_logger.h"
#include "src/movement/move_types.h"
#include <src/engine/zobrist_hashing.h>

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
        const auto startTime = std::chrono::system_clock::now();
        setupTimeControls(startTime, board);

        return scanForBestMove(startTime, depthInput.value_or(s_maxSearchDepth), board);
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
            uint64_t oldHash = m_hash;
            const auto newBoard = engine::performMove(board, move, m_hash);

            int16_t score = -negamax(depth, newBoard);
            m_hash = oldHash;

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
        m_isStopped = false;

        m_whiteTime = 0;
        m_blackTime = 0;
        m_movesToGo = 0;
        m_moveTime.reset();
        m_whiteMoveInc = 0;
        m_blackMoveInc = 0;
    }

private:
    constexpr void setupTimeControls(std::chrono::time_point<std::chrono::system_clock> start, const BitBoard& board)
    {
        using namespace std::chrono;

        const auto timeLeft = board.player == Player::White ? milliseconds(m_whiteTime) : milliseconds(m_blackTime);
        const auto timeInc = board.player == Player::White ? milliseconds(m_whiteMoveInc) : milliseconds(m_blackMoveInc);
        const auto buffer = timeBuffer(board);

        if (m_moveTime) {
            m_endTime = start + milliseconds(m_moveTime.value()) - buffer + timeInc;
        } else if (m_movesToGo) {
            const auto time = timeLeft / m_movesToGo;
            m_endTime = start + time - buffer + timeInc;
        } else {
            /* TODO: how long should we search if no time controls are set? */
            const auto time = timeLeft / s_defaultAmountMoves;
            m_endTime = start + time - buffer + timeInc;
        }

        /* just to make sure that we actually search something - better to run out of time than to not search any moves.. */
        if (m_endTime <= start) {
            m_endTime = start + buffer + timeInc;
        }
    };

    constexpr movement::Move scanForBestMove(std::chrono::time_point<std::chrono::system_clock> startTime, uint8_t depth, const BitBoard& board)
    {
        using namespace std::chrono;

        int16_t alpha = s_minScore;
        int16_t beta = s_maxScore;

        /*
         * iterative deeping - with aspiration window
         * https://web.archive.org/web/20070705134903/www.seanet.com/%7Ebrucemo/topics/aspiration.htm
         */
        for (uint8_t d = 1; d <= depth; d++) {
            if (m_isStopped)
                break;

            m_scoring.pvTable().setIsFollowing(true);

            int16_t score = negamax(d, board, alpha, beta);

            /* the search fell outside the window - we need to retry a full search */
            if ((score <= alpha) || (score >= beta)) {
                alpha = s_minScore;
                beta = s_maxScore;
                continue;
            }

            /* prepare window for next iteration */
            alpha = score - s_aspirationWindow;
            beta = score + s_aspirationWindow;

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
        const auto hashScore = engine::tt::readHashEntry(m_hash, alpha, beta, depth);
        if (hashScore.has_value()) {
            return hashScore.value();
        }

        using namespace std::chrono;
        checkIfStopped();

        m_scoring.pvTable().updateLength(m_ply);

        if (depth == 0) {
            return quiesence(board, alpha, beta);
        }

        // Engine is not designed to search deeper than this! Make sure to stop before it's too late
        if (m_ply >= s_maxSearchDepth) {
            return materialScore(board);
        }

        m_nodes++;

        engine::tt::TtHashFlag hashFlag = engine::tt::TtHashAlpha;
        int16_t score = 0;
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

            /* enPessant is invalid if we skip move */
            boardCopy.enPessant.reset();

            /* give opponent an extra move */
            boardCopy.player = nextPlayer(boardCopy.player);

            m_ply++;

            /* perform search with reduced depth (based on reduction limit) */
            score = -negamax(depth - 1 - s_nullMoveReduction, boardCopy, -beta, -beta + 1);

            m_ply--;

            if (m_isStopped)
                return 0;

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
            /* TODO: make this smarter.. un/do move? */
            uint64_t oldHash = m_hash;
            const auto newBoard = engine::performMove(board, move, m_hash);
            if (engine::isKingAttacked(newBoard, currentPlayer)) {
                m_hash = oldHash;
                // invalid move
                continue;
            }

            m_ply++;
            legalMoves++;

            const auto startTime = system_clock::now();
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

            m_hash = oldHash;
            printMoveInfo(move, startTime);

            m_ply--;

            if (m_isStopped)
                return 0;

            movesSearched++;

            if (score >= beta) {
                engine::tt::writeHashEntry(m_hash, score, depth, engine::tt::TtHashBeta);
                m_scoring.killerMoves().update(move, m_ply);
                return beta;
            }

            if (score > alpha) {
                alpha = score;
                hashFlag = engine::tt::TtHashExact;

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

        engine::tt::writeHashEntry(m_hash, score, depth, hashFlag);
        return alpha;
    }

    void
    printMoveInfo(const movement::Move& move, const std::chrono::time_point<std::chrono::system_clock>& startTime)
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
        using namespace std::chrono;
        checkIfStopped();

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
            uint64_t oldHash = m_hash;
            const auto newBoard = engine::performMove(board, move, m_hash);

            if (engine::isKingAttacked(newBoard, board.player)) {
                m_hash = oldHash;
                // invalid move
                continue;
            }

            m_ply++;

            const auto startTime = system_clock::now();
            const int16_t score = -quiesence(newBoard, -beta, -alpha);

            m_hash = oldHash;
            printMoveInfo(move, startTime);

            m_ply--;

            if (m_isStopped)
                return 0;

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

    constexpr void checkIfStopped()
    {
        /* only check every 2048 nodes */
        if ((m_nodes & 2047) != 0)
            return;

        if (std::chrono::system_clock::now() > m_endTime) {
            m_isStopped = true;
        }
    }

    constexpr std::chrono::milliseconds timeBuffer(const BitBoard& board)
    {
        using namespace std::chrono_literals;

        if (board.fullMoves < 4) {
            return 500ms;
        } else if (board.fullMoves < 6) {
            return 250ms;
        } else {
            return 5ms;
        }
    }

    FileLogger& m_logger;

    uint64_t m_nodes {};
    uint8_t m_ply;
    bool m_isStopped;
    uint64_t m_hash {};

    MoveScoring m_scoring {};

    uint64_t m_whiteTime {};
    uint64_t m_blackTime {};
    uint64_t m_movesToGo {};
    std::optional<uint64_t> m_moveTime {};
    uint64_t m_whiteMoveInc {};
    uint64_t m_blackMoveInc {};

    std::chrono::time_point<std::chrono::system_clock> m_endTime;

    /* search configs */
    constexpr static inline uint16_t s_fullDepthMove { 4 };
    constexpr static inline uint16_t s_reductionLimit { 3 };
    constexpr static inline uint16_t s_defaultAmountMoves { 50 };

    constexpr static inline uint8_t s_aspirationWindow { 50 };
    constexpr static inline uint8_t s_nullMoveReduction { 2 };
};
}
