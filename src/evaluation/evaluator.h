#pragma once

#include "engine/thread_pool.h"
#include "engine/tt_hash_table.h"
#include "evaluation/searcher.h"
#include "time_manager.h"

#include "fmt/ranges.h"
#include "movegen/move_types.h"
#include <atomic>
#include <engine/zobrist_hashing.h>

#include <chrono>

namespace evaluation {

class Evaluator {
public:
    constexpr uint64_t getNodes() const
    {
        return m_searcher.getNodes();
    }

    constexpr movegen::Move getBestMove(const BitBoard& board, std::optional<uint8_t> depthInput = std::nullopt)
    {

        /* if a depth has been provided then make sure that we search to that depth */
        if (m_isPondering || depthInput.has_value()) {
            TimeManager::startInfinite();
        } else {
            TimeManager::start(board, m_threadPool);
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

            TimeManager::start(board, m_threadPool);
        }
    }

    void setPondering(bool enabled)
    {
        m_ponderingEnabled = enabled;
    }

    void stop()
    {
        m_isPondering = false;
        TimeManager::stop();
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

    /*
     * This method will reset search parameters
     * to be called before starting a scan
     */
    void resetTiming()
    {
        TimeManager::reset();
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

    constexpr void printScoreInfo(Searcher* searcher, const BitBoard& board, int32_t score, uint8_t currentDepth)
    {
        using namespace std::chrono;

        const auto timeDiff = TimeManager::timeElapsedMs().count();

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
            if (d > 1 && !TimeManager::timeForAnotherSearch(board)) {
                break;
            }

            const int32_t score = m_searcher.startSearch(d, board, alpha, beta);

            if ((score <= alpha) || (score >= beta)) {
                alpha = s_minScore;
                beta = s_maxScore;

                continue;
            }

            /* prepare window for next iteration */
            alpha = score - s_aspirationWindow;
            beta = score + s_aspirationWindow;

            printScoreInfo(&m_searcher, board, score, d);

            /* only increment depth, if we didn't fall out of the window */
            d++;
        }

        /* in case we're scanning to a certain depth we need to ensure that we're stopping the time handler */
        stop();

        return m_searcher.getPvMove();
    }

    Searcher m_searcher {};
    std::atomic_bool m_killed { false };

    ThreadPool m_threadPool { 2 }; /* Search and time handler */
    bool m_isPondering { false };
    bool m_ponderingEnabled { false };

    constexpr static inline uint8_t s_aspirationWindow { 50 };
};
}
