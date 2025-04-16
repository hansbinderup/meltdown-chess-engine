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
#include <map>

#include <chrono>
#include <condition_variable>
#include <future>
#include <mutex>
#include <thread>

namespace evaluation {

enum SearchType {
    Default,
    NullSearch,
};

enum SearchState {
    Idle,
    Searching,
    Kill
};

struct MoveResult {
    uint64_t hash;
    BitBoard board;
};

struct SearcherResult {
    int32_t score;
    uint8_t searcherId;
    movegen::Move pvMove;
    uint8_t searchedDepth;
};

template<typename T, uint8_t maxSize>
class StackVector {
public:
    uint8_t size() { return m_size; }

    T& at(uint8_t idx) { return m_entries.at(idx); }

    void setSize(uint8_t size) { m_size = size; }

    void incSize() { m_size++; }

    auto* begin() { return m_entries.begin(); }

    auto* end() { return m_entries.begin() + m_size; }

private:
    std::array<T, maxSize> m_entries;
    uint8_t m_size { 0 };
};

template<typename T, typename U, uint8_t maxSize>
class StackMap {
public:
    StackMap() = default;

    StackMap(std::initializer_list<std::pair<T, U>> init)
    {
        assert(init.size() < maxSize);

        for (auto& entry : init) {
            m_entries.at(m_size++) = std::move(entry);
        }
    }

    constexpr U& at(const T& ext_key)
    {
        for (auto& [key, value] : m_entries) {
            if (key == ext_key) {
                return value;
            }
        }
        throw std::out_of_range("No entry matching this key");
    }

    constexpr bool count(const T& ext_key)
    {
        for (auto& [key, value] : m_entries) {
            if (key == ext_key) {
                return true;
            }
        }
        return false;
    }

    template<class... Args>
    constexpr void insert(Args&&... args)
    {
        assert(m_size < maxSize);
        m_entries.at(m_size++) = std::pair<T, U>(std::forward<Args>(args)...);
    }

    uint8_t size() const { return m_size; }

    constexpr void clear()
    {
        for (uint8_t i = 0; i < m_size; i++) {
            m_entries.at(i) = std::pair<T, U> {};
        }
        m_size = 0;
    }

    auto* begin() { return m_entries.begin(); }

    auto* end() { return m_entries.begin() + m_size; }

private:
    std::array<std::pair<T, U>, maxSize> m_entries {};
    uint8_t m_size {};
};

/* Base class for searcher */
class Searcher {
public:
    Searcher()
        : m_searcherId(s_numSearchers++) {};

    Searcher(const Searcher&) = delete;

    void killWorkerThread()
    {
        if (!m_threadActive || !m_workerThread.joinable())
            return;

        {
            std::unique_lock<std::mutex> lock(m_mtx);
            m_searchState.store(SearchState::Kill);
        }
        m_cv.notify_all();
        m_workerThread.join();
        m_threadActive = false;
    }

    ~Searcher()
    {
        killWorkerThread();

        s_numSearchers--;
    }

    Searcher& operator=(const Searcher&) = delete;

    uint8_t getSearcherId()
    {
        return m_searcherId;
    }

    bool getThreadActive()
    {
        return m_threadActive;
    }

    void startThreadLoop()
    {
        m_workerThread = std::thread([this]() { this->threadLoop(); });
        m_threadActive = true;
    }

    void setSmpBoard(const BitBoard& board)
    {
        m_board = board;
    }

    void setHashKey(uint64_t hash)
    {
        m_hash = hash;
    }

    void startSearch(uint8_t depth, int32_t alpha, int32_t beta)
    {
        s_oneFullSearchComplete.store(false, std::memory_order_relaxed);

        m_moveOrdering.pvTable().setIsFollowing(true);
        m_wdl = syzygy::WdlResultTableNotActive;
        m_dtz = 0;

        {
            std::unique_lock<std::mutex> lock(m_mtx);

            m_searchPromise = std::promise<SearcherResult>();
            m_futureResult = m_searchPromise.get_future();

            m_searchDepth = depth;
            m_alpha = alpha;
            m_beta = beta;

            if (m_searchState.load(std::memory_order_relaxed) != SearchState::Kill) {
                m_searchState.store(SearchState::Searching, std::memory_order_relaxed);
            }
        }
        m_cv.notify_one();
    }

    constexpr SearcherResult getSearchResult()
    {
        return m_futureResult.get();
    }

    constexpr movegen::Move getPvMove() const
    {
        return m_moveOrdering.pvTable().bestMove();
    }

    constexpr std::optional<movegen::Move>
    getPonderMove() const
    {
        if (m_moveOrdering.pvTable().size() > 1) {
            return m_moveOrdering.pvTable()[1];
        }

        return std::nullopt;
    }

    void
    resetTiming()
    {
        s_nodes = 0;
        m_selDepth = 0;
    }

    void reset()
    {
        m_moveOrdering.reset();
        m_repetition.reset();
        m_selDepth = 0;
        m_ttMove.reset();

        m_wdl = syzygy::WdlResultTableNotActive;
        m_dtz = 0;

        m_board.reset();
        m_hash = 0;
        m_searchDepth = 0;
        m_alpha = s_minScore;
        m_beta = s_maxScore;
    }

    void updateRepetition(uint64_t hash)
    {
        m_repetition.add(hash);
    }

    constexpr void printScoreInfo(const BitBoard& board, int32_t score, uint8_t currentDepth, const TimePoint& startTime)
    {
        using namespace std::chrono;

        if (m_searchState.load(std::memory_order_relaxed) == SearchState::Kill) {
            return;
        }

        const auto endTime = system_clock::now();
        const auto timeDiff = duration_cast<milliseconds>(endTime - startTime).count();

        uint8_t tbHit = 0;
        score = syzygy::approximateDtzScore(board, score, m_dtz, m_wdl, tbHit);

        if (score > -s_mateValue && score < -s_mateScore)
            fmt::print("info score mate {} time {} depth {} seldepth {} nodes {} tbhits {} pv ", -(score + s_mateValue) / 2 - 1, timeDiff, currentDepth, m_selDepth, s_nodes, tbHit);
        else if (score > s_mateScore && score < s_mateValue)
            fmt::print("info score mate {} time {} depth {} seldepth {} nodes {} tbhits {} pv ", (s_mateValue - score) / 2 + 1, timeDiff, currentDepth, m_selDepth, s_nodes, tbHit);
        else
            fmt::print("info score cp {} time {} depth {} seldepth {} nodes {} hashfull {} tbhits {} pv ", score, timeDiff, currentDepth, m_selDepth, s_nodes, engine::TtHashTable::getHashFull(), tbHit);

        fmt::println("{}", fmt::join(m_moveOrdering.pvTable(), " "));

        fflush(stdout);
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
        s_isStopped.store(false, std::memory_order_relaxed);

        movegen::ValidMoves moves;
        engine::getAllMoves<movegen::MovePseudoLegal>(board, moves);
        m_moveOrdering.sortMoves(board, moves, m_ply);
        const int32_t score = negamax(depth, board);
        s_isStopped = true;

        fmt::println("Move evaluations [{}]:", depth);
        for (const auto& move : moves) {
            fmt::println("  {}: {}", move, m_moveOrdering.moveScore(board, move, 0));
        }

        fmt::println("\nTotal nodes:     {}\n"
                     "Search score:    {}\n"
                     "PV-line:         {}\n"
                     "Static eval:     {}\n",
            s_nodes, score, fmt::join(m_moveOrdering.pvTable(), " "), staticEvaluation(board));
    }

    template<SearchType searchType = SearchType::Default>
    constexpr int32_t negamax(uint8_t depth, const BitBoard& board, int32_t alpha = s_minScore, int32_t beta = s_maxScore)
    {
        if (s_oneFullSearchComplete) {
            return alpha;
        }

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

        s_nodes++;

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

            if (s_isStopped.load(std::memory_order_relaxed) || m_searchState.load(std::memory_order_relaxed) == SearchState::Kill)
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

    static std::atomic_bool s_isStopped;
    static std::atomic_bool s_oneFullSearchComplete;

private:
    void threadLoop()
    {
        while (true) {
            std::unique_lock<std::mutex> lock(m_mtx);
            m_cv.wait(lock, [this] {
                const auto state = m_searchState.load(std::memory_order_relaxed);
                return state == SearchState::Searching || state == SearchState::Kill;
            });

            if (m_searchState == SearchState::Kill) {
                return;
            }

            SearcherResult result;
            result.score
                = negamax(m_searchDepth, m_board, m_alpha, m_beta);
            result.searcherId = m_searcherId;
            result.pvMove = m_moveOrdering.pvTable().bestMove();
            result.searchedDepth = m_moveOrdering.pvTable().size();

            m_searchPromise.set_value(std::move(result));
            s_oneFullSearchComplete.store(true, std::memory_order_relaxed);
            if (m_searchState.load(std::memory_order_relaxed) != SearchState::Kill) {
                m_searchState.store(SearchState::Idle);
            }
        }
    }

    constexpr int32_t quiesence(const BitBoard& board, int32_t alpha, int32_t beta)
    {
        using namespace std::chrono;

        s_nodes++;
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

            if (s_isStopped.load(std::memory_order_relaxed) || m_searchState == SearchState::Kill)
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

    static uint8_t s_numSearchers;
    static uint64_t s_nodes;

    uint8_t m_searcherId {};
    uint8_t m_ply {};
    Repetition m_repetition;
    MoveOrdering m_moveOrdering {};
    uint8_t m_selDepth {};
    std::optional<movegen::Move> m_ttMove {};

    std::thread m_workerThread;
    std::mutex m_mtx;
    std::condition_variable m_cv;
    std::atomic<SearchState> m_searchState { SearchState::Idle };
    bool m_threadActive { false };

    syzygy::WdlResult m_wdl = syzygy::WdlResultTableNotActive;
    uint8_t m_dtz {};

    BitBoard m_board;
    uint64_t m_hash;
    uint8_t m_searchDepth;
    int32_t m_alpha { s_minScore };
    int32_t m_beta { s_maxScore };

    std::promise<SearcherResult> m_searchPromise;
    std::future<SearcherResult> m_futureResult;

    /* search configs */
    constexpr static inline uint32_t s_fullDepthMove { 4 };
    constexpr static inline uint32_t s_reductionLimit { 3 };
    constexpr static inline int32_t s_futilityMargin { 100 };
    constexpr static inline int32_t s_futilityEvaluationMargin { 120 };
    constexpr static inline int32_t s_razorMarginShallow { 125 };
    constexpr static inline int32_t s_razorMarginDeep { 175 };
    constexpr static inline uint8_t s_razorDeepReductionLimit { 2 };

    constexpr static inline uint8_t s_nullMoveReduction { 2 };
};

std::atomic_bool Searcher::s_isStopped { true };
std::atomic_bool Searcher::s_oneFullSearchComplete { false };
uint8_t Searcher::s_numSearchers { 0 };
uint64_t Searcher::s_nodes { 0 };

class Evaluator {
public:
    Evaluator()
    {
        m_searchers.incSize();
    };

    void resizeSearchers(uint8_t size)
    {
        if (size == m_searchers.size()) {
            return;
        }

        if (size < m_searchers.size()) {
            for (uint8_t i = size; i < m_searchers.size(); i++) {
                m_searchers.at(i).killWorkerThread();
                m_searchers.at(i).reset();
            }
        }

        m_searchers.setSize(size);
    }

    constexpr movegen::Move
    getBestMove(const BitBoard& board, std::optional<uint8_t> depthInput = std::nullopt)
    {
        for (auto& searcher : m_searchers) {
            if (!searcher.getThreadActive()) {
                searcher.startThreadLoop();
            }
        }

        m_startTime
            = std::chrono::system_clock::now();

        /*
         * if a depth has been provided then make sure that we search to that depth
         * timeout will be 10 minutes
         */
        if (depthInput.has_value()) {
            m_endTime = m_startTime + std::chrono::minutes(10);
        } else {
            setupTimeControls(m_startTime, board);
        }

        uint64_t hash = engine::generateHashKey(board);

        for (auto& searcher : m_searchers) {
            searcher.setHashKey(hash);
            searcher.setSmpBoard(board);
        }

        return scanForBestMove(depthInput.value_or(s_maxSearchDepth), board);
    }

    constexpr std::optional<movegen::Move> getPonderMove() const
    {
        return m_ponderMove;
    }

    /* TODO: ensure time handler is stopped as well - not just assumed stopped. This is racy */
    constexpr void stop()
    {
        Searcher::s_isStopped.store(true, std::memory_order_relaxed);
    }

    constexpr void terminate()
    {
        for (auto& searcher : m_searchers) {
            while (searcher.getThreadActive()) {
                searcher.killWorkerThread();
            }
        }
    }

    constexpr void printEvaluation(const BitBoard& board, std::optional<uint8_t> depthInput = std::nullopt)
    {
        // Placeholder: print per-searcher for now
        Searcher::s_isStopped.store(false, std::memory_order_relaxed);
        for (auto& searcher : m_searchers) {
            searcher.printEvaluation(board, depthInput);
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

        for (auto& searcher : m_searchers) {
            searcher.resetTiming();
        }
    }

    void reset()
    {
        resetTiming(); // full reset required
        for (auto& searcher : m_searchers) {
            searcher.reset();
        }
    }

    void updateRepetition(uint64_t hash)
    {
        for (auto& searcher : m_searchers) {
            searcher.updateRepetition(hash);
        }
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
        movegen::Move bestMove;
        uint8_t d = 1;

        while (d <= depth) {
            if (Searcher::s_isStopped.load(std::memory_order_relaxed))
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

            /* Thread voting: https://www.chessprogramming.org/Lazy_SMP */
            StackVector<SearcherResult, s_maxThreads> searchResults {};

            for (auto& searcher : m_searchers) {
                searcher.startSearch(d, alpha, beta);
            }

            for (auto& searcher : m_searchers) {
                const auto& result = searcher.getSearchResult();

                // If didn't fall out of window and wasn't an immediate early termination, push back to results
                if ((result.score > alpha) && (result.score < beta) && result.searchedDepth > 0) {
                    searchResults.at(searchResults.size()) = std::move(result);
                    searchResults.incSize();
                }
            }
            if (searchResults.size() == 0) {
                alpha = s_minScore;
                beta = s_maxScore;

                continue;
            }

            m_movesVotes.clear();

            for (const auto& result : searchResults) {

                const int32_t score = result.score;
                const uint8_t depth = result.searchedDepth;
                const auto move = result.pvMove;

                /* Can be tweaked for optimization */
                int64_t voteWeight = (score - s_minScore) * depth;

                if (!m_movesVotes.count(move)) {
                    m_movesVotes.insert(move, voteWeight);
                } else {
                    m_movesVotes.at(move) += voteWeight;
                }
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

            for (const auto& result : searchResults) {
                if (result.pvMove == bestMove) {
                    if (depth > bestWinningDepth) {
                        bestWinningResult = result;
                        bestWinningDepth = depth;
                    }
                }
            }

            /* prepare window for next iteration */
            alpha = bestWinningResult.score - s_aspirationWindow;
            beta = bestWinningResult.score + s_aspirationWindow;

            for (auto& searcher : m_searchers) {
                if (searcher.getSearcherId() == bestWinningResult.searcherId) {
                    searcher.printScoreInfo(board, bestWinningResult.score, d, m_startTime);
                    m_ponderMove = searcher.getPonderMove();
                }
            }

            /* only increment depth, if we didn't fall out of the window */
            d++;
        }

        /* in case we're scanning to a certain depth we need to ensure that we're stopping the time handler */
        stop();

        return bestMove;
    }

    void startTimeHandler()
    {
        bool wasStopped = Searcher::s_isStopped.exchange(false);
        if (!wasStopped) {
            return; /* nothing to do here */
        }

        std::thread timeHandler([this] {
            while (!Searcher::s_isStopped.load(std::memory_order_relaxed)) {
                {
                    std::scoped_lock lock(m_timeHandleMutex);
                    if (Searcher::s_isStopped.load(std::memory_order_relaxed))
                        return; /* stopped elsewhere */

                    if (std::chrono::system_clock::now() > m_endTime) {
                        Searcher::s_isStopped.store(true, std::memory_order_relaxed);
                    }
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });

        timeHandler.detach();
    }

    StackVector<Searcher, s_maxThreads> m_searchers {};

    uint64_t m_whiteTime {};
    uint64_t m_blackTime {};
    uint64_t m_movesToGo {};
    std::optional<uint64_t> m_moveTime {};
    uint64_t m_whiteMoveInc {};
    uint64_t m_blackMoveInc {};

    StackMap<movegen::Move, int64_t, s_maxThreads> m_movesVotes {};

    std::optional<movegen::Move> m_ponderMove { std::nullopt };

    TimePoint m_startTime;
    TimePoint m_endTime;
    std::mutex m_timeHandleMutex;

    constexpr static inline uint32_t s_defaultAmountMoves { 75 };
    constexpr static inline uint8_t s_aspirationWindow { 50 };
};
}
