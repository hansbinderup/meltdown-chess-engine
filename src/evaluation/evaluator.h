#pragma once

#include "core/thread_pool.h"
#include "core/time_manager.h"
#include "core/transposition.h"
#include "core/zobrist_hashing.h"
#include "evaluation/move_vote_map.h"
#include "interface/outputs.h"
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
                m_searchers.emplace_back(Searcher::create());
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

        return startIterativeDeepening(depthInput.value_or(s_maxSearchDepth), board);
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

    constexpr movegen::Move startIterativeDeepening(uint8_t depth, const BitBoard& board)
    {
        const movegen::Move bestMove = m_searchers.size() == 1
            ? iterativeDeepeningSingle(depth, board)
            : iterativeDeepeningMulti(depth, board);

        stop();

        return bestMove;
    }

    struct AspirationWindow {
        Score alpha;
        Score beta;
        Score delta;
        uint8_t depthReduction;
        const uint8_t depth;

        explicit AspirationWindow(uint8_t depth, Score prevScore)
            : depth(depth)
        {
            if (depth >= spsa::aspirationMinDepth) {
                alpha = std::max<Score>(s_minScore, prevScore - spsa::aspirationWindow);
                beta = std::min<Score>(s_maxScore, prevScore + spsa::aspirationWindow);
            } else {
                alpha = s_minScore;
                beta = s_maxScore;
            }
            delta = spsa::aspirationWindow;
            depthReduction = 0;
        }

        inline void widenOnFailLow()
        {
            alpha = std::max<Score>(s_minScore, alpha - delta);
            beta = (alpha + beta) / 2;
            depthReduction = 0;
        }

        inline void widenOnFailHigh()
        {
            beta = std::min<Score>(s_maxScore, beta + delta);
            depthReduction++;
        }

        inline void grow()
        {
            delta *= 2;
            if (delta > spsa::aspirationMaxWindow) {
                alpha = s_minScore;
                beta = s_maxScore;
                depthReduction = 0;
            }
        }

        inline uint8_t searchDepth() const
        {
            /* noia check - don't want to overflow nor search depth 0 */
            return depthReduction < depth
                ? depth - depthReduction
                : 1;
        }
    };

    constexpr movegen::Move iterativeDeepeningSingle(uint8_t depth, const BitBoard& board)
    {
        const auto& singleSearcher = m_searchers.front();
        Score bestScore = 0;
        movegen::Move bestMove;

        for (uint8_t d = 1; d <= depth; d++) {
            if (!TimeManager::timeForAnotherSearch(d)) {
                break;
            }

            AspirationWindow window(d, bestScore);

            while (true) {
                Searcher::setSearchStopped(false);

                const auto score = singleSearcher->startSearch(window.searchDepth(), board, window.alpha, window.beta);

                if (TimeManager::hasTimedOut()) {
                    break;
                }

                if (score <= window.alpha) {
                    window.widenOnFailLow();
                } else if (score >= window.beta) {
                    window.widenOnFailHigh();
                } else {
                    bestScore = score;
                    interface::printSearchInfo(singleSearcher, score, d, getNodes(), getTbHits());
                    bestMove = singleSearcher->getPvMove();
                    m_ponderMove = singleSearcher->getPonderMove();
                    TimeManager::updateMoveStability(bestMove, score, pvMoveNodeFraction(bestMove));

                    break;
                }

                window.grow();
            }
        }

        return bestMove;
    }

    /* currently using a more primitive iterative deepening with fixed window search
     * update to use similar implementation as single threaded search */
    constexpr movegen::Move iterativeDeepeningMulti(uint8_t depth, const BitBoard& board)
    {
        movegen::Move bestMove;
        uint8_t d = 1;
        Score alpha = s_minScore;
        Score beta = s_maxScore;

        while (d <= depth) {
            if (!TimeManager::timeForAnotherSearch(d)) {
                break;
            }

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

            /* use previous search if we timed out - it means that the current search was incomplete */
            if (TimeManager::hasTimedOut()) {
                break;
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

            const auto searcher = bestWinningResult.searcher.lock();
            if (!searcher) {
                /* should not happen during a search - have we been stopped? */
                break;
            }

            interface::printSearchInfo(searcher, bestWinningResult.score, d, getNodes(), getTbHits());
            m_ponderMove = searcher->getPonderMove();

            TimeManager::updateMoveStability(bestMove, bestWinningResult.score, pvMoveNodeFraction(bestMove));

            d++;
        }

        return bestMove;
    }

    std::atomic_bool m_killed { false };

    ThreadPool m_threadPool { 3 }; /* Iterative deepening, time handler, default=1 searcher */

    std::vector<std::shared_ptr<Searcher>> m_searchers {};
    MoveVoteMap<s_maxThreads> m_movesVotes {};

    bool m_isPondering { false };
    bool m_ponderingEnabled { false };
    std::optional<movegen::Move> m_ponderMove {};
};
}
