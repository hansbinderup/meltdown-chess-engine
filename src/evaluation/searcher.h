#pragma once

#include "engine/move_handling.h"
#include "engine/thread_pool.h"
#include "engine/tt_hash_table.h"
#include "evaluation/move_ordering.h"
#include "evaluation/pv_table.h"
#include "evaluation/repetition.h"
#include "evaluation/static_evaluation.h"
#include "time_manager.h"

#include "fmt/ranges.h"
#include "movegen/move_types.h"
#include "syzygy/syzygy.h"
#include <engine/zobrist_hashing.h>

#include <future>

namespace evaluation {

enum SearchType {
    Default,
    NullSearch,
};

struct MoveResult {
    uint64_t hash;
    BitBoard board;
};

class Searcher;

struct SearcherResult {
    Score score;
    movegen::Move pvMove;
    uint8_t searchedDepth;
    Searcher* searcher;
};

class Searcher {
public:
    Searcher() = default;

    void setHashKey(uint64_t hash)
    {
        m_hash = hash;
    }

    constexpr uint64_t getNodes() const
    {
        return m_nodes;
    }

    constexpr uint8_t getSelDepth() const
    {
        return m_selDepth;
    }

    static constexpr void setSearchStopped(bool value)
    {
        s_searchStopped.store(value, std::memory_order_relaxed);
    }

    Score inline startSearch(uint8_t depth, const BitBoard& board, Score alpha, Score beta)
    {
        m_moveOrdering.pvTable().setIsFollowing(true);
        m_wdl = syzygy::WdlResultTableNotActive;
        m_dtz = 0;

        return negamax(depth, board, alpha, beta);
    }

    void inline startSearchAsync(ThreadPool& threadPool, uint8_t depth, const BitBoard& board, Score alpha, Score beta)
    {
        m_searchPromise = std::promise<SearcherResult> {};
        m_futureResult = m_searchPromise.get_future();

        m_moveOrdering.pvTable().setIsFollowing(true);
        m_wdl = syzygy::WdlResultTableNotActive;
        m_dtz = 0;

        const bool started = threadPool.submit([this, depth, board, alpha, beta] {
            SearcherResult result {
                .score
                = negamax(depth, board, alpha, beta),
                .pvMove = m_moveOrdering.pvTable().bestMove(),
                .searchedDepth = m_moveOrdering.pvTable().size(),
                .searcher = this
            };

            /* Stop any searches going on in other threads, so we're not waiting on the slowest search */
            setSearchStopped(true);

            m_searchPromise.set_value(result);
        });

        assert(started);
    }

    constexpr std::optional<SearcherResult> getSearchResult()
    {
        return m_futureResult.get();
    }

    constexpr movegen::Move getPvMove() const
    {
        return m_moveOrdering.pvTable().bestMove();
    }

    constexpr std::optional<movegen::Move> getPonderMove() const
    {
        const auto ponderMove = m_moveOrdering.pvTable().ponderMove();
        return ponderMove.isNull() ? std::nullopt : std::make_optional(ponderMove);
    }

    void resetNodes()
    {
        m_nodes = 0;
        m_selDepth = 0;
    }

    void reset()
    {
        m_moveOrdering.reset();
        m_repetition.reset();
        resetNodes();

        m_wdl = syzygy::WdlResultTableNotActive;
        m_dtz = 0;

        m_board.reset();
        m_hash = 0;
    }

    void updateRepetition(uint64_t hash)
    {
        m_repetition.add(hash);
    }

    constexpr std::pair<Score, uint8_t> approxDtzScore(const BitBoard& board, Score score)
    {
        uint8_t tbHit = 0;
        score = syzygy::approximateDtzScore(board, score, m_dtz, m_wdl, tbHit);
        return { score, tbHit };
    }

    constexpr auto& getPvTable() const
    {
        return m_moveOrdering.pvTable();
    }

    constexpr void printEvaluation(const BitBoard& board, std::optional<uint8_t> depthInput = std::nullopt)
    {
        resetNodes(); /* to reset nodes etc but not tables */

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

        TimeManager::startInfinite();

        movegen::ValidMoves moves;
        engine::getAllMoves<movegen::MovePseudoLegal>(board, moves);
        m_moveOrdering.sortMoves(board, moves, m_ply);
        const Score score = negamax(depth, board);

        TimeManager::stop();

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

    template<SearchType searchType = SearchType::Default>
    constexpr Score negamax(uint8_t depth, const BitBoard& board, Score alpha = s_minScore, Score beta = s_maxScore)
    {
        const bool isRoot = m_ply == 0;

        m_moveOrdering.pvTable().updateLength(m_ply);
        if (m_ply) {
            /* FIXME: make a little more sophisticated with material count etc */
            const bool isDraw = m_repetition.isRepetition(m_hash) || board.halfMoves >= 100;
            if (isDraw) {
                /* draw score is 0 but to avoid blindness towards three fold lines
                 * we add a slight variance to the draw score
                 * it will still be approx 0~ cp: [-0.1:0.1] */
                return 1 - (m_nodes & 2); /* draw score */
            }
        }

        const bool isPv = beta - alpha > 1;
        const auto hashProbe = engine::TtHashTable::probe(m_hash);
        if (hashProbe.has_value() && m_ply && !isPv) {
            const auto testResult = engine::testEntry(*hashProbe, m_ply, depth, alpha, beta);
            if (testResult.has_value()) {
                return testResult.value();
            }
        }

        // Engine is not designed to search deeper than this! Make sure to stop before it's too late
        if (m_ply >= s_maxSearchDepth) {
            return staticEvaluation(board);
        }

        const bool isChecked = engine::isKingAttacked(board);
        if (isChecked) {
            /* Dangerous position - increase search depth
             * NOTE: there's rarely many legal moves in this position
             * so it's quite cheap to extend the search a bit */
            depth++;
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

        const Score staticEval = staticEvaluation(board);

        /* https://www.chessprogramming.org/Reverse_Futility_Pruning */
        if (depth < s_reductionLimit && !isPv && !isChecked) {
            const bool withinFutilityMargin = abs(beta - 1) > (s_minScore + s_futilityMargin);
            const Score evalMargin = s_futilityEvaluationMargin * depth;

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
            Score score = staticEval + s_razorMarginShallow;
            if (score < beta) {
                if (depth == 1) {
                    Score newScore = quiesence(board, alpha, beta);
                    return (newScore > score) ? newScore : score;
                }

                score += s_razorMarginDeep;
                if (score < beta && depth <= s_razorDeepReductionLimit) {
                    const Score newScore = quiesence(board, alpha, beta);
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
                Score score = 0;
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

            const auto ttMove = hashProbe.has_value() ? std::make_optional(hashProbe->move) : std::nullopt;
            m_moveOrdering.sortMoves(board, moves, m_ply, ttMove);
        }

        for (const auto& move : moves) {
            const auto moveRes = makeMove(board, move);
            if (!moveRes.has_value()) {
                continue;
            }

            Score score = 0;
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

            if (isSearchStopped())
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

private:
    constexpr Score quiesence(const BitBoard& board, Score alpha, Score beta)
    {
        using namespace std::chrono;

        m_nodes++;
        m_selDepth = std::max(m_selDepth, m_ply);

        const Score evaluation = staticEvaluation(board);

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

            const Score score = -quiesence(moveRes->board, -beta, -alpha);
            undoMove(moveRes->hash);

            if (isSearchStopped())
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
    std::optional<Score> nullMovePruning(const BitBoard& board, uint8_t depth, Score beta)
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
        Score score = -negamax<SearchType::NullSearch>(depth - 1 - s_nullMoveReduction, nullMoveBoard, -beta, -beta + 1);

        m_hash = oldHash;
        m_ply -= 2;
        m_repetition.remove();

        if (score >= beta) {
            return beta;
        }

        return std::nullopt;
    }

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

    inline bool isSearchStopped() const
    {
        if (s_searchStopped.load(std::memory_order_relaxed))
            return true;

        if (m_nodes % 2048 == 0) {
            TimeManager::updateTimeout();
        }

        return TimeManager::hasTimedOut();
    }

    static inline uint8_t s_numSearchers {};
    static inline std::atomic_bool s_searchStopped { true };

    uint64_t m_nodes {};
    uint8_t m_ply {};
    Repetition m_repetition;
    MoveOrdering m_moveOrdering {};
    uint8_t m_selDepth {};

    syzygy::WdlResult m_wdl { syzygy::WdlResultTableNotActive };
    uint8_t m_dtz {};

    BitBoard m_board;
    uint64_t m_hash;

    std::promise<SearcherResult> m_searchPromise;
    std::future<SearcherResult> m_futureResult;

    /* search configs */
    constexpr static inline uint32_t s_fullDepthMove { 4 };
    constexpr static inline uint32_t s_reductionLimit { 3 };
    constexpr static inline Score s_futilityMargin { 100 };
    constexpr static inline Score s_futilityEvaluationMargin { 120 };
    constexpr static inline Score s_razorMarginShallow { 125 };
    constexpr static inline Score s_razorMarginDeep { 175 };
    constexpr static inline uint8_t s_razorDeepReductionLimit { 2 };

    constexpr static inline uint8_t s_nullMoveReduction { 2 };
};

}
