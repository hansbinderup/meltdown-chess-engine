#pragma once

#include "engine/thread_pool.h"
#include "engine/tt_hash_table.h"
#include "evaluation/searcher.h"

#include "fmt/ranges.h"
#include "movegen/move_types.h"
#include <atomic>
#include <engine/zobrist_hashing.h>

#include <chrono>
#include <mutex>

namespace evaluation {

class Evaluator {
public:
    constexpr uint64_t getNodes() const
    {
        return m_searcher.getNodes();
    }

    constexpr movegen::Move getBestMove(const BitBoard& board, std::optional<uint8_t> depthInput = std::nullopt)
    {
        m_startTime = std::chrono::system_clock::now();
        m_isStopped = false;

        /* if a depth has been provided then make sure that we search to that depth */
        if (m_isPondering || depthInput.has_value()) {
            m_endTime = TimePoint::max();
        } else {
            setupTimeControls(m_startTime, board);
            startTimeHandler();
        }

        const uint64_t hash = engine::generateHashKey(board);

        m_searcher.setHashKey(hash);

        return scanForBestMove(depthInput.value_or(s_maxSearchDepth), board);
    }

    constexpr bool startPondering(const BitBoard& board)
    {
        if (m_isPondering || !m_ponderingEnabled)
            return false;

        m_isPondering = true;
        return getBestMoveAsync(board);
    }

    constexpr bool getBestMoveAsync(const BitBoard& board, std::optional<uint8_t> depthInput = std::nullopt)
    {
        return m_threadPool.submit([this, board, depthInput] {
            const auto move = getBestMove(board, depthInput);

            if (m_killed.load(std::memory_order_relaxed))
                return;

            fmt::print("bestmove {}", move);
            if (m_ponderingEnabled) {
                const auto ponderMove = m_searcher.getPonderMove();
                if (ponderMove.has_value()) {
                    fmt::print(" ponder {}", ponderMove.value());
                }
            }

            fmt::println("");
            fflush(stdout);
        });
    }

    void onPonderHit(const BitBoard& board)
    {
        if (m_ponderingEnabled) {
            m_isPondering = false;

            const auto now = std::chrono::system_clock::now();
            setupTimeControls(now, board);
            startTimeHandler();
        }
    }

    void setPondering(bool enabled)
    {
        m_ponderingEnabled = enabled;
    }

    void stop()
    {
        m_isPondering = false;
        m_isStopped.store(true, std::memory_order_relaxed);
        Searcher::setSearchStopped(true);
    }

    void kill()
    {
        m_killed.store(true, std::memory_order_relaxed);
        stop();
    }

    constexpr void printEvaluation(const BitBoard& board, std::optional<uint8_t> depthInput = std::nullopt)
    {
        m_searcher.printEvaluation(board, depthInput);
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

        m_searcher.resetNodes();
    }

    void reset()
    {
        resetTiming(); // full reset required
        m_searcher.reset();
    }

    void updateRepetition(uint64_t hash)
    {
        m_searcher.updateRepetition(hash);
    }

    constexpr void printScoreInfo(Searcher* searcher, const BitBoard& board, int32_t score, uint8_t currentDepth, const TimePoint& startTime)
    {
        using namespace std::chrono;

        const auto endTime = system_clock::now();
        const auto timeDiff = duration_cast<milliseconds>(endTime - startTime).count();

        auto scoreTbHit = searcher->approxDtzScore(board, score);
        score = scoreTbHit.first;
        uint8_t tbHit = scoreTbHit.second;

        if (score > -s_mateValue && score < -s_mateScore)
            fmt::print("info score mate {} time {} depth {} seldepth {} nodes {} tbhits {} pv ", -(score + s_mateValue) / 2 - 1, timeDiff, currentDepth, searcher->getSelDepth(), getNodes(), tbHit);
        else if (score > s_mateScore && score < s_mateValue)
            fmt::print("info score mate {} time {} depth {} seldepth {} nodes {} tbhits {} pv ", (s_mateValue - score) / 2 + 1, timeDiff, currentDepth, searcher->getSelDepth(), getNodes(), tbHit);
        else
            fmt::print("info score cp {} time {} depth {} seldepth {} nodes {} hashfull {} tbhits {} pv ", score, timeDiff, currentDepth, searcher->getSelDepth(), getNodes(), engine::TtHashTable::getHashFull(), tbHit);

        fmt::println("{}", fmt::join(searcher->getPvTable(), " "));

        fflush(stdout);
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
    }

    constexpr movegen::Move scanForBestMove(uint8_t depth, const BitBoard& board)
    {
        int32_t alpha = s_minScore;
        int32_t beta = s_maxScore;

        /*
         * iterative deeping - with aspiration window
         * https://web.archive.org/web/20070705134903/www.seanet.com/%7Ebrucemo/topics/aspiration.htm
         */
        uint8_t d = 1;

        while (d <= depth) {
            if (d > 1) {
                if (m_isStopped.load(std::memory_order_relaxed)) {
                    break;
                }

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
            }

            Searcher::setSearchStopped(false);
            const int32_t score = m_searcher.startSearch(d, board, alpha, beta);

            if ((score <= alpha) || (score >= beta)) {
                alpha = s_minScore;
                beta = s_maxScore;

                continue;
            }

            /* prepare window for next iteration */
            alpha = score - s_aspirationWindow;
            beta = score + s_aspirationWindow;

            printScoreInfo(&m_searcher, board, score, d, m_startTime);

            /* only increment depth, if we didn't fall out of the window */
            d++;
        }

        /* in case we're scanning to a certain depth we need to ensure that we're stopping the time handler */
        stop();

        return m_searcher.getPvMove();
    }

    void startTimeHandler()
    {
        /* we only have three slots - so if we can't start the thread then it must already be running */
        std::ignore = m_threadPool.submit([this] {
            while (!m_isStopped.load(std::memory_order_relaxed)) {
                {
                    std::scoped_lock lock(m_timeHandleMutex);
                    if (m_isStopped.load(std::memory_order_relaxed))
                        return; /* stopped elsewhere */

                    if (std::chrono::system_clock::now() > m_endTime) {
                        stop();
                    }
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });
    }

    Searcher m_searcher {};

    std::atomic_bool m_isStopped { true };
    std::atomic_bool m_killed { false };

    uint64_t m_whiteTime {};
    uint64_t m_blackTime {};
    uint64_t m_movesToGo {};
    std::optional<uint64_t> m_moveTime {};
    uint64_t m_whiteMoveInc {};
    uint64_t m_blackMoveInc {};

    TimePoint m_startTime;
    TimePoint m_endTime;
    std::mutex m_timeHandleMutex;

    ThreadPool m_threadPool { 2 }; /* Search and time handler */
    bool m_isPondering { false };
    bool m_ponderingEnabled { false };

    constexpr static inline uint32_t s_defaultAmountMoves { 75 };
    constexpr static inline uint8_t s_aspirationWindow { 50 };
};
}
