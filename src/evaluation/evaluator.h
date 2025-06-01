#pragma once

#include "core/thread_pool.h"
#include "core/time_manager.h"
#include "core/transposition.h"
#include "core/zobrist_hashing.h"
#include "evaluation/move_vote_map.h"
#include "movegen/move_types.h"
#include "search/searcher.h"

#include "fmt/ranges.h"
#include <atomic>

#include <chrono>

namespace evaluation {

using search::Searcher;
using search::SearcherResult;

class Evaluator {
public:
    Evaluator()
    {
        resizeSearchers(1);
    };

    void resizeSearchers(size_t size)
    {
        if (size == m_searchers.size()) {
            return;
        } else {
            for (size_t i = m_searchers.size(); i < size; i++) {
                m_searchers.emplace_back(std::make_unique<Searcher>());
            }
        }

        m_searchers.resize(size);
        m_threadPool.resize(size + 2);

        bool primarySearcher = true;
        for (auto& searcher : m_searchers) {
            searcher->setIsPrimary(std::exchange(primarySearcher, false));
        }
    }

    constexpr uint64_t getNodes() const
    {
        uint64_t totalNodes {};
        for (auto& searcher : m_searchers) {
            totalNodes += searcher->getNodes();
        }
        return totalNodes;
    }

    constexpr uint64_t getTbHits() const
    {
        uint64_t totalTbHits {};
        for (const auto& searcher : m_searchers) {
            totalTbHits += searcher->getTbHits();
        }
        return totalTbHits;
    }

    constexpr uint64_t getHistoryNodes(movegen::Move move) const
    {
        uint64_t totalNodes {};
        for (const auto& searcher : m_searchers) {
            totalNodes += searcher->getHistoryNodes(move);
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

        const uint64_t hash = core::generateHashKey(board);

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

    constexpr void printScoreInfo(Searcher* searcher, const BitBoard& board, Score score, uint8_t currentDepth)
    {
        const auto timeDiff = TimeManager::timeElapsedMs().count();

        const auto adjustedScore = searcher->approxDtzScore(board, score);
        const uint64_t nodes = getNodes();
        const uint64_t tbHits = getTbHits();
        const uint16_t hashFull = core::TranspositionTable::getHashFull();

        fmt::print("info score {} time {} depth {} seldepth {} nodes {} hashfull {}{}{} pv ",
            ScorePrint(adjustedScore),
            timeDiff,
            currentDepth,
            searcher->getSelDepth(),
            nodes,
            hashFull,
            NpsPrint(nodes, timeDiff),
            TbHitPrint(tbHits));

        fmt::println("{}", fmt::join(searcher->getPvTable(), " "));

        fflush(stdout);
    }

private:
    constexpr double pvMoveNodeFraction(movegen::Move pvMove)
    {
        const uint64_t totalNodes = getNodes();
        const uint64_t pvNodes = getHistoryNodes(pvMove);

        if (totalNodes <= 0) {
            return 1.0;
        }

        return static_cast<double>(pvNodes) / totalNodes;
    }

    constexpr movegen::Move scanForBestMove(uint8_t depth, const BitBoard& board)
    {
        Score alpha = s_minScore;
        Score beta = s_maxScore;

        /*
         * iterative deeping - with aspiration window
         * https://web.archive.org/web/20070705134903/www.seanet.com/%7Ebrucemo/topics/aspiration.htm
         */
        movegen::Move bestMove;
        uint8_t d = 1;

        while (d <= depth) {
            if (!TimeManager::timeForAnotherSearch(d)) {
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
                alpha = score - spsa::aspirationWindow;
                beta = score + spsa::aspirationWindow;

                printScoreInfo(singleSearcher.get(), board, score, d);

                bestMove = singleSearcher->getPvMove();
                m_ponderMove = singleSearcher->getPonderMove();

                TimeManager::updateMoveStability(bestMove, score, pvMoveNodeFraction(bestMove));
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

                    const Score score = result.score;
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
                alpha = bestWinningResult.score - spsa::aspirationWindow;
                beta = bestWinningResult.score + spsa::aspirationWindow;

                printScoreInfo(bestWinningResult.searcher, board, bestWinningResult.score, d);
                m_ponderMove = bestWinningResult.searcher->getPonderMove();

                TimeManager::updateMoveStability(bestMove, bestWinningResult.score, pvMoveNodeFraction(bestMove));
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
};
}
