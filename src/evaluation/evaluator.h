#pragma once

#include "engine/thread_pool.h"
#include "engine/tt_hash_table.h"
#include "evaluation/move_vote_map.h"
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
    Evaluator()
    {
        resizeSearchers(1);
    };

    void resizeSearchers(uint8_t size)
    {
        if (size == m_searchers.size()) {
            return;
        }

        else {
            for (uint8_t i = m_searchers.size(); i < size; i++) {
                m_searchers.emplace_back(std::make_unique<Searcher>());
            }
        }

        m_searchers.resize(size);
        m_threadPool.resize(size + 2);
    }

    constexpr uint64_t getNodes() const
    {
        uint64_t totalNodes {};
        for (auto& searcher : m_searchers) {
            totalNodes += searcher->getNodes();
        }
        return totalNodes;
    }

    constexpr movegen::Move getBestMove(const BitBoard& board, std::optional<uint8_t> depthInput = std::nullopt)
    {

        /* if a depth has been provided then make sure that we search to that depth */
        if (m_isPondering || depthInput.has_value()) {
            TimeManager::startInfinite();
        } else {
            TimeManager::start(board);
        }

        const uint64_t hash = engine::generateHashKey(board);

        for (auto& searcher : m_searchers) {
            searcher->setHashKey(hash);
        }

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
                if (m_ponderMove.has_value()) {
                    fmt::print(" ponder {}", m_ponderMove.value());
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

            TimeManager::start(board);
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
        Searcher::setSearchStopped(true);
    }

    void kill()
    {
        m_killed.store(true, std::memory_order_relaxed);
        stop();
    }

    constexpr void printEvaluation(const BitBoard& board, std::optional<uint8_t> depthInput = std::nullopt)
    {
        for (const auto& searcher : m_searchers) {
            searcher->printEvaluation(board, depthInput);
        }
    }

    /*
     * This method will reset search parameters
     * to be called before starting a scan
     */
    void resetTiming()
    {
        TimeManager::reset();

        for (auto& searcher : m_searchers) {
            searcher->resetNodes();
        }
    }

    void reset()
    {
        resetTiming(); // full reset required
        for (auto& searcher : m_searchers) {
            searcher->reset();
        }
    }

    void updateRepetition(uint64_t hash)
    {
        for (auto& searcher : m_searchers) {
            searcher->updateRepetition(hash);
        }
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

    constexpr static inline uint8_t s_maxThreads { 128 };

private:
    constexpr movegen::Move scanForBestMove(uint8_t depth, const BitBoard& board)
    {
        int32_t alpha = s_minScore;
        int32_t beta = s_maxScore;

        /*
         * iterative deeping - with aspiration window
         * https://web.archive.org/web/20070705134903/www.seanet.com/%7Ebrucemo/topics/aspiration.htm
         */
        movegen::Move bestMove;
        uint8_t d = 1;

        while (d <= depth) {
            if (d > 1 && !TimeManager::timeForAnotherSearch()) {
                break;
            }

            if (m_searchers.size() == 1) {
                const auto& singleSearcher = m_searchers.front();

                Searcher::setSearchStopped(false);
                const auto score = singleSearcher->startSearch(d, board, alpha, beta);

                if ((score <= alpha) || (score >= beta)) {
                    alpha = s_minScore;
                    beta = s_maxScore;

                    continue;
                }

                /* prepare window for next iteration */
                alpha = score - s_aspirationWindow;
                beta = score + s_aspirationWindow;

                printScoreInfo(singleSearcher.get(), board, score, d);

                bestMove = singleSearcher->getPvMove();
                m_ponderMove = singleSearcher->getPonderMove();
            } else {
                Searcher::setSearchStopped(false);

                /* Thread voting: https://www.chessprogramming.org/Lazy_SMP */
                std::array<SearcherResult, s_maxThreads> searchResults {};
                uint8_t numSearchResults {};

                for (auto& searcher : m_searchers) {
                    searcher->startSearchAsync(m_threadPool, d, board, alpha, beta);
                }

                for (auto& searcher : m_searchers) {
                    const auto& result = searcher->getSearchResult();
                    if (result.has_value()) {
                        // If didn't fall out of window and wasn't an immediate early termination, push back to results
                        if ((result.value().score > alpha) && (result.value().score < beta) && result.value().searchedDepth > 0) {
                            searchResults.at(numSearchResults) = result.value();
                            ++numSearchResults;
                        }
                    }
                }

                if (numSearchResults == 0) {
                    alpha = s_minScore;
                    beta = s_maxScore;

                    continue;
                }

                m_movesVotes.clear();

                for (uint8_t i = 0; i < numSearchResults; i++) {

                    const auto& result = searchResults.at(i);

                    const int32_t score = result.score;
                    const uint8_t depth = result.searchedDepth;
                    const auto move = result.pvMove;

                    /* Can be tweaked for optimization */
                    int64_t voteWeight = (score - s_minScore) * depth;

                    m_movesVotes.insertOrIncrement(move, voteWeight);
                }

                int64_t maxVote = -std::numeric_limits<int64_t>::max();

                for (const auto& [move, vote] : m_movesVotes) {
                    if (vote > maxVote) {
                        maxVote = vote;
                        bestMove = move;
                    }
                }

                uint8_t bestWinningDepth = 0;
                SearcherResult bestWinningResult;

                for (uint8_t i = 0; i < numSearchResults; i++) {
                    if (searchResults.at(i).pvMove == bestMove) {
                        if (depth > bestWinningDepth) {
                            bestWinningResult = searchResults.at(i);
                            bestWinningDepth = depth;
                        }
                    }
                }

                /* prepare window for next iteration */
                alpha = bestWinningResult.score - s_aspirationWindow;
                beta = bestWinningResult.score + s_aspirationWindow;

                printScoreInfo(bestWinningResult.searcher, board, bestWinningResult.score, d);
                m_ponderMove = bestWinningResult.searcher->getPonderMove();
            }

            /* only increment depth, if we didn't fall out of the window */
            d++;
        }

        /* in case we're scanning to a certain depth we need to ensure that we're stopping the time handler */
        stop();

        return bestMove;
    }

    Searcher m_searcher {};
    std::atomic_bool m_killed { false };

    ThreadPool m_threadPool { 3 }; /* Iterative deepening, time handler, default=1 searcher */

    std::vector<std::unique_ptr<Searcher>> m_searchers {};
    MoveVoteMap<s_maxThreads> m_movesVotes {};

    bool m_isPondering { false };
    bool m_ponderingEnabled { false };
    std::optional<movegen::Move> m_ponderMove {};

    constexpr static inline uint8_t s_aspirationWindow { 50 };
};
}
