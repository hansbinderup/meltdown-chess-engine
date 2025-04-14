#pragma once

#include "engine/move_handling.h"
#include "engine/tt_hash_table.h"
#include "evaluation/move_ordering.h"
#include "evaluation/pv_table.h"
#include "evaluation/repetition.h"
#include "evaluation/static_evaluation.h"
#include "fmt/ranges.h"
#include "movegen/move_types.h"
#include "syzygy/syzygy.h"
#include <atomic>
#include <engine/zobrist_hashing.h>

#include <chrono>
#include <mutex>
#include <thread>

namespace evaluation {

class Evaluator {
public:
    constexpr movegen::Move getBestMove(const BitBoard& board, std::optional<uint8_t> depthInput = std::nullopt)
    {
        m_startTime = std::chrono::system_clock::now();

        /*
         * if a depth has been provided then make sure that we search to that depth
         * timeout will be 10 minutes
         */
        if (depthInput.has_value()) {
            m_endTime = m_startTime + std::chrono::minutes(10);
        } else {
            setupTimeControls(m_startTime, board);
        }

        m_hash = engine::generateHashKey(board);

        return scanForBestMove(depthInput.value_or(s_maxSearchDepth), board);
    }

    constexpr std::optional<movegen::Move> getPonderMove() const
    {
        if (m_moveOrdering.pvTable().size() > 1) {
            return m_moveOrdering.pvTable()[1];
        }

        return std::nullopt;
    }

    /* TODO: ensure time handler is stopped as well - not just assumed stopped. This is racy */
    constexpr void stop()
    {
        m_isStopped = true;
    }

    constexpr uint64_t getNodes() const
    {
        return m_nodes;
    }

    constexpr void printEvaluation(const BitBoard& board, std::optional<uint8_t> depthInput = std::nullopt)
    {
        resetTiming(); /* to reset nodes etc but not tables */

        uint8_t depth = depthInput.value_or(5);

        fmt::println("");

        movegen::ValidMoves captures;
        engine::getAllMoves<movegen::MoveCapture>(board, captures);
        if (captures.count()) {
            m_moveOrdering.sortMoves(board, captures, m_ply);

            fmt::print("Captures[{}]: ", captures.count());
            for (const auto& move : captures) {
                fmt::print("{} [{}]  ", move, m_moveOrdering.moveScore(board, move, 0));
            }
            fmt::print("\n\n");
        }

        /* no need for time management here - just control the start/stop ourselves */
        m_isStopped = false;

        movegen::ValidMoves moves;
        engine::getAllMoves<movegen::MovePseudoLegal>(board, moves);
        m_moveOrdering.sortMoves(board, moves, m_ply);
        const int32_t score = negamax(depth, board);
        stop();

        fmt::println("Move evaluations [{}]:", depth);
        for (const auto& move : moves) {
            fmt::println("  {}: {}", move, m_moveOrdering.moveScore(board, move, 0));
        }

        fmt::println("\nTotal nodes:     {}\n"
                     "Search score:    {}\n"
                     "PV-line:         {}\n"
                     "Static eval:     {}\n",
            m_nodes, score, fmt::join(m_moveOrdering.pvTable(), " "), staticEvaluation(board));
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

    /*
     * This method will reset search parameters
     * to be called before starting a scan
     */
    void resetTiming()
    {
        m_whiteTime = 0;
        m_blackTime = 0;
        m_movesToGo = 0;
        m_moveTime.reset();
        m_whiteMoveInc = 0;
        m_blackMoveInc = 0;
        m_nodes = 0;
        m_selDepth = 0;
    }

    void reset()
    {
        resetTiming(); // full reset required
        m_moveOrdering.reset();
        m_repetition.reset();
    }

    void updateRepetition(uint64_t hash)
    {
        m_repetition.add(hash);
    }

    void ponderhit(const BitBoard& board)
    {
        setupTimeControls(m_startTime, board);
    }

private:
    constexpr void setupTimeControls(TimePoint start, const BitBoard& board)
    {
        std::scoped_lock lock(m_timeHandleMutex);

        using namespace std::chrono;

        /* allow some extra time for processing etc */
        constexpr auto buffer = 50ms;

        const auto timeLeft = board.player == PlayerWhite ? milliseconds(m_whiteTime) : milliseconds(m_blackTime);
        const auto timeInc = board.player == PlayerWhite ? milliseconds(m_whiteMoveInc) : milliseconds(m_blackMoveInc);

        if (m_moveTime) {
            m_endTime = start + milliseconds(m_moveTime.value()) - buffer + timeInc;
        } else if (m_movesToGo) {
            const auto time = timeLeft / m_movesToGo;
            m_endTime = start + time - buffer + timeInc;
        } else {
            /* Dynamic time allocation based on game phase */
            constexpr uint32_t openingMoves = 20;
            constexpr uint32_t lateGameMoves = 50;

            /* Estimate remaining moves */
            uint32_t movesRemaining = std::clamp(s_defaultAmountMoves - board.fullMoves, openingMoves, lateGameMoves);

            /* Allocate time proportionally */
            const auto time = timeLeft / movesRemaining;

            /* Adjust based on game phase (early = slightly faster, late = slightly deeper) */
            const float phaseFactor = 1.0 + (static_cast<float>(board.fullMoves) / s_defaultAmountMoves) * 0.5;
            const auto adjustedTime = duration_cast<milliseconds>(time * phaseFactor);

            m_endTime = start + adjustedTime - buffer + timeInc;
        }

        /* just to make sure that we actually search something - better to run out of time than to not search any moves.. */
        if (m_endTime <= start) {
            m_endTime = start + milliseconds(250) + timeInc;
        }
    };

    constexpr movegen::Move scanForBestMove(uint8_t depth, const BitBoard& board)
    {
        startTimeHandler();

        int32_t alpha = s_minScore;
        int32_t beta = s_maxScore;

        /*
         * iterative deeping - with aspiration window
         * https://web.archive.org/web/20070705134903/www.seanet.com/%7Ebrucemo/topics/aspiration.htm
         */

        uint8_t d = 1;
        while (d <= depth) {
            if (m_isStopped)
                break;

            /* always allow full scan on first move - will be good for the hash table :) */
            if (board.fullMoves > 0) {
                const auto now = std::chrono::system_clock::now();
                const auto timeLeft = m_endTime - now;
                const auto timeSpent = now - m_startTime;

                /* factor is "little less than half" meaning that we give juuust about half the time we spent to search a new depth
                 * might need tweaking - will do when game phases are implemented */
                const auto timeLimit = timeSpent / 1.9;

                /* uncommment for debugging */

                /* m_logger.log("d: {}, timeLeft: {}, timeSpent: {}, timeLimit: {}", d, */
                /*     std::chrono::duration_cast<std::chrono::milliseconds>(timeLeft), */
                /*     std::chrono::duration_cast<std::chrono::milliseconds>(timeSpent), */
                /*     std::chrono::duration_cast<std::chrono::milliseconds>(timeLimit)); */

                if (timeLeft < timeLimit) {
                    /* m_logger.log("Stopped early; saved: {}", std::chrono::duration_cast<std::chrono::milliseconds>(timeLeft)); */
                    break;
                }
            }

            m_moveOrdering.pvTable().setIsFollowing(true);
            m_wdl = syzygy::WdlResultTableNotActive;
            m_dtz = 0;

            int32_t score = negamax(d, board, alpha, beta);

            /* the search fell outside the window - we need to retry a full search */
            if ((score <= alpha) || (score >= beta)) {
                alpha = s_minScore;
                beta = s_maxScore;

                continue;
            }

            /* prepare window for next iteration */
            alpha = score - s_aspirationWindow;
            beta = score + s_aspirationWindow;

            printScoreInfo(board, score, d);

            /* only increment depth, if we didn't fall out of the window */
            d++;
        }

        /* in case we're scanning to a certain depth we need to ensure that we're stopping the time handler */
        stop();

        return m_moveOrdering.pvTable().bestMove();
    }

    constexpr void printScoreInfo(const BitBoard& board, int32_t score, uint8_t currentDepth)
    {
        using namespace std::chrono;

        const auto endTime = system_clock::now();
        const auto timeDiff = duration_cast<milliseconds>(endTime - m_startTime).count();

        uint8_t tbHit = 0;
        score = syzygy::approximateDtzScore(board, score, m_dtz, m_wdl, tbHit);

        if (score > -s_mateValue && score < -s_mateScore)
            fmt::print("info score mate {} time {} depth {} seldepth {} nodes {} tbhits {} pv ", -(score + s_mateValue) / 2 - 1, timeDiff, currentDepth, m_selDepth, m_nodes, tbHit);
        else if (score > s_mateScore && score < s_mateValue)
            fmt::print("info score mate {} time {} depth {} seldepth {} nodes {} tbhits {} pv ", (s_mateValue - score) / 2 + 1, timeDiff, currentDepth, m_selDepth, m_nodes, tbHit);
        else
            fmt::print("info score cp {} time {} depth {} seldepth {} nodes {} hashfull {} tbhits {} pv ", score, timeDiff, currentDepth, m_selDepth, m_nodes, engine::TtHashTable::getHashFull(), tbHit);

        fmt::println("{}", fmt::join(m_moveOrdering.pvTable(), " "));

        fflush(stdout);
    }

    enum SearchType {
        Default,
        NullSearch,
    };

    template<SearchType searchType = SearchType::Default>
    constexpr int32_t negamax(uint8_t depth, const BitBoard& board, int32_t alpha = s_minScore, int32_t beta = s_maxScore)
    {
        const bool isRoot = m_ply == 0;

        m_moveOrdering.pvTable().updateLength(m_ply);
        if (m_ply && m_repetition.isRepetition(m_hash)) {
            return 0; /* draw score */
        }

        const bool isPv = beta - alpha > 1;
        const auto hashProbe = engine::TtHashTable::probe(m_hash, alpha, beta, depth, m_ply);
        if (hashProbe.has_value()) {
            if (m_ply && !isPv)
                return hashProbe->first;

            m_ttMove = hashProbe->second;
        } else {
            m_ttMove.reset();
        }

        // Engine is not designed to search deeper than this! Make sure to stop before it's too late
        if (m_ply >= s_maxSearchDepth) {
            return staticEvaluation(board);
        }

        if (depth == 0) {
            return quiesence(board, alpha, beta);
        }

        m_nodes++;

        /* entries for the TT hash */
        engine::TtHashFlag hashFlag = engine::TtHashAlpha;
        movegen::Move alphaMove {};

        uint32_t legalMoves = 0;
        uint64_t movesSearched = 0;
        const bool isChecked = engine::isKingAttacked(board);

        // Dangerous position - increase search depth
        if (isChecked) {
            depth++;
        }

        const int32_t staticEval = staticEvaluation(board);

        /* https://www.chessprogramming.org/Reverse_Futility_Pruning */
        if (depth < s_reductionLimit && !isPv && !isChecked) {
            const bool withinFutilityMargin = abs(beta - 1) > (s_minScore + s_futilityMargin);
            const int32_t evalMargin = s_futilityEvaluationMargin * depth;

            if (withinFutilityMargin && (staticEval - evalMargin) >= beta)
                return staticEval - evalMargin;
        }

        /* dangerous to repeat null search on a null search - skip it here */
        if constexpr (searchType != SearchType::NullSearch) {
            if (depth > s_nullMoveReduction && !isChecked && m_ply) {
                if (const auto nullMoveScore = nullMovePruning(board, depth, beta)) {
                    return nullMoveScore.value();
                }
            }
        }

        /* https://www.chessprogramming.org/Razoring (Strelka) */
        if (!isPv && !isChecked && depth <= s_reductionLimit) {
            int32_t score = staticEval + s_razorMarginShallow;
            if (score < beta) {
                if (depth == 1) {
                    int32_t newScore = quiesence(board, alpha, beta);
                    return (newScore > score) ? newScore : score;
                }

                score += s_razorMarginDeep;
                if (score < beta && depth <= s_razorDeepReductionLimit) {
                    const int32_t newScore = quiesence(board, alpha, beta);
                    if (newScore < beta)
                        return (newScore > score) ? newScore : score;
                }
            }
        }

        bool tbMoves = false;
        movegen::ValidMoves moves {};
        if (syzygy::isTableActive(board)) {
            if (isRoot) {
                tbMoves = syzygy::generateSyzygyMoves(board, moves, m_wdl, m_dtz);
            } else if (board.isQuietPosition()) {
                int32_t score = 0;
                syzygy::probeWdl(board, score);
                engine::TtHashTable::writeEntry(m_hash, score, movegen::Move {}, s_maxSearchDepth, m_ply, engine::TtHashExact);
                return score;
            }
        }

        if (!tbMoves) {
            engine::getAllMoves<movegen::MovePseudoLegal>(board, moves);

            if (m_moveOrdering.pvTable().isFollowing()) {
                m_moveOrdering.pvTable().updatePvScoring(moves, m_ply);
            }

            m_moveOrdering.sortMoves(board, moves, m_ply, m_ttMove);
        }

        for (const auto& move : moves) {
            const auto moveRes = makeMove(board, move);
            if (!moveRes.has_value()) {
                continue;
            }

            int32_t score = 0;
            legalMoves++;

            /*
             * LMR
             * https://wiki.sharewiz.net/doku.php?id=chess:programming:late_move_reduction
             */
            if (movesSearched == 0) {
                score = -negamax(depth - 1, moveRes->board, -beta, -alpha);
            } else {
                if (movesSearched >= s_fullDepthMove
                    && depth >= s_reductionLimit
                    && !isChecked
                    && !move.isCapture()
                    && !move.isPromotionMove()) {
                    /* search current move with reduced depth */
                    score = -negamax(depth - 2, moveRes->board, -alpha - 1, -alpha);
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
                    score = -negamax(depth - 1, moveRes->board, -alpha - 1, -alpha);

                    /* if it failed high, do a full re-search */
                    if ((score > alpha) && (score < beta)) {
                        score = -negamax(depth - 1, moveRes->board, -beta, -alpha);
                    }
                }
            }

            undoMove(moveRes->hash);

            if (m_isStopped)
                return score;

            if (score >= beta) {
                engine::TtHashTable::writeEntry(m_hash, score, move, depth, m_ply, engine::TtHashBeta);
                m_moveOrdering.killerMoves().update(move, m_ply);
                return beta;
            }

            movesSearched++;
            if (score > alpha) {
                alpha = score;

                hashFlag = engine::TtHashExact;
                alphaMove = move;

                m_moveOrdering.historyMoves().update(board, move, m_ply);
                m_moveOrdering.pvTable().updateTable(move, m_ply);
            }
        }

        if (legalMoves == 0) {
            if (isChecked) {
                // We want absolute negative score - but with amount of moves to the given checkmate
                // we add the ply to make checkmate in less moves a better move
                return -s_mateValue + m_ply;
            } else {
                // Stalemate - absolute neutral score
                return 0;
            }
        }

        engine::TtHashTable::writeEntry(m_hash, alpha, alphaMove, depth, m_ply, hashFlag);
        return alpha;
    }

    constexpr int32_t quiesence(const BitBoard& board, int32_t alpha, int32_t beta)
    {
        using namespace std::chrono;

        m_nodes++;
        m_selDepth = std::max(m_selDepth, m_ply);

        const int32_t evaluation = staticEvaluation(board);

        if (m_ply >= s_maxSearchDepth)
            return evaluation;

        // Hard cutoff
        if (evaluation >= beta) {
            return beta;
        }

        if (evaluation > alpha) {
            alpha = evaluation;
        }

        movegen::ValidMoves moves;
        engine::getAllMoves<movegen::MoveCapture>(board, moves);
        m_moveOrdering.sortMoves(board, moves, m_ply);

        for (const auto& move : moves) {
            const auto moveRes = makeMove(board, move);
            if (!moveRes.has_value()) {
                continue;
            }

            const int32_t score = -quiesence(moveRes->board, -beta, -alpha);
            undoMove(moveRes->hash);

            if (m_isStopped)
                return score;

            if (score >= beta)
                // change to beta for hard cutoff
                return beta;

            if (score > alpha) {
                alpha = score;
            }
        }

        return alpha;
    }

    /*
     * null move pruning
     * https://www.chessprogramming.org/Null_Move_Pruning
     * */
    std::optional<int32_t> nullMovePruning(const BitBoard& board, uint8_t depth, int32_t beta)
    {
        auto nullMoveBoard = board;
        uint64_t oldHash = m_hash;
        if (nullMoveBoard.enPessant.has_value())
            engine::hashEnpessant(nullMoveBoard.enPessant.value(), m_hash);

        engine::hashPlayer(m_hash);

        /* enPessant is invalid if we skip move */
        nullMoveBoard.enPessant.reset();

        /* give opponent an extra move */
        nullMoveBoard.player = nextPlayer(nullMoveBoard.player);

        m_ply += 2;
        m_repetition.add(oldHash);

        /* perform search with reduced depth (based on reduction limit) */
        int32_t score = -negamax<SearchType::NullSearch>(depth - 1 - s_nullMoveReduction, nullMoveBoard, -beta, -beta + 1);

        m_hash = oldHash;
        m_ply -= 2;
        m_repetition.remove();

        if (score >= beta) {
            return beta;
        }

        return std::nullopt;
    }

    struct MoveResult {
        uint64_t hash;
        BitBoard board;
    };

    std::optional<MoveResult> makeMove(const BitBoard& board, const movegen::Move& move)
    {
        const MoveResult res { m_hash, engine::performMove(board, move, m_hash) };

        if (engine::isKingAttacked(res.board, board.player)) {
            m_hash = res.hash;

            // invalid move
            return std::nullopt;
        }

        m_repetition.add(res.hash);
        m_ply++;

        return res;
    }

    void undoMove(uint64_t hash)
    {
        m_hash = hash;

        m_repetition.remove();
        m_ply--;
    }

    void startTimeHandler()
    {
        bool wasStopped = m_isStopped.exchange(false);
        if (!wasStopped) {
            return; /* nothing to do here */
        }

        std::thread timeHandler([this] {
            while (!m_isStopped) {
                {
                    std::scoped_lock lock(m_timeHandleMutex);
                    if (m_isStopped)
                        return; /* stopped elsewhere */

                    if (std::chrono::system_clock::now() > m_endTime) {
                        m_isStopped = true;
                    }
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });

        timeHandler.detach();
    }

    uint64_t m_nodes {};
    uint8_t m_ply {};
    std::atomic_bool m_isStopped { true };
    uint64_t m_hash {};
    uint8_t m_selDepth {};
    std::optional<movegen::Move> m_ttMove;

    syzygy::WdlResult m_wdl = syzygy::WdlResultTableNotActive;
    uint8_t m_dtz {};

    MoveOrdering m_moveOrdering {};
    Repetition m_repetition;

    uint64_t m_whiteTime {};
    uint64_t m_blackTime {};
    uint64_t m_movesToGo {};
    std::optional<uint64_t> m_moveTime {};
    uint64_t m_whiteMoveInc {};
    uint64_t m_blackMoveInc {};

    TimePoint m_startTime;
    TimePoint m_endTime;
    std::mutex m_timeHandleMutex;

    /* search configs */
    constexpr static inline uint32_t s_fullDepthMove { 4 };
    constexpr static inline uint32_t s_reductionLimit { 3 };
    constexpr static inline int32_t s_futilityMargin { 100 };
    constexpr static inline int32_t s_futilityEvaluationMargin { 120 };
    constexpr static inline int32_t s_razorMarginShallow { 125 };
    constexpr static inline int32_t s_razorMarginDeep { 175 };
    constexpr static inline uint8_t s_razorDeepReductionLimit { 2 };
    constexpr static inline uint32_t s_defaultAmountMoves { 75 };

    constexpr static inline uint8_t s_aspirationWindow { 50 };
    constexpr static inline uint8_t s_nullMoveReduction { 2 };
};
}
