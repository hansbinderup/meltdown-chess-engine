#pragma once

#include "core/move_handling.h"
#include "core/thread_pool.h"
#include "core/time_manager.h"
#include "core/transposition.h"
#include "evaluation/static_evaluation.h"
#include "search/lmr_table.h"
#include "search/move_picker.h"
#include "search/repetition.h"
#include "search/search_tables.h"
#include "spsa/parameters.h"

#include "core/zobrist_hashing.h"
#include "movegen/move_types.h"
#include "syzygy/syzygy.h"

#include <future>

namespace search {

struct MoveResult {
    uint64_t hash;
    BitBoard board;
};

class Searcher;

struct SearcherResult {
    Score score;
    movegen::Move pvMove;
    uint8_t searchedDepth;
    std::weak_ptr<Searcher> searcher;
};

class Searcher : public std::enable_shared_from_this<Searcher> {

private:
    /* enable_shared_from_this only works if the object itself is a shared_ptr
     * therefore hide constructor and provide a factory instead */
    Searcher() = default;

public:
    static std::shared_ptr<Searcher> create()
    {
        /* std::make_shared doesn't work with default constructor not available - create manually */
        return std::shared_ptr<Searcher>(new Searcher());
    }

    void setIsPrimary(bool isPrimary)
    {
        m_isPrimary = isPrimary;
    }

    constexpr uint64_t getNodes() const
    {
        return m_nodes;
    }

    constexpr uint64_t getTbHits() const
    {
        return m_tbHits;
    }

    constexpr uint64_t getHistoryNodes(movegen::Move move)
    {
        return m_searchTables.getHistoryNodes(move);
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
        assert(m_stackItr == m_stack.begin());

        m_stack.front().board = board;

        return negamax<true, true>(depth, board, alpha, beta);
    }

    void inline startSearchAsync(ThreadPool& threadPool, uint8_t depth, const BitBoard& board, Score alpha, Score beta)
    {
        assert(m_stackItr == m_stack.begin());

        m_searchPromise = std::promise<SearcherResult> {};
        m_futureResult = m_searchPromise.get_future();

        m_stack.front().board = board;

        [[maybe_unused]] const bool started = threadPool.submit([this, depth, board, alpha, beta] {
            SearcherResult result {
                .score = negamax<true, true>(depth, board, alpha, beta),
                .pvMove = m_searchTables.getBestPvMove(),
                .searchedDepth = m_searchTables.getPvSize(),
                .searcher = weak_from_this()
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
        return m_searchTables.getBestPvMove();
    }

    constexpr std::optional<movegen::Move> getPonderMove() const
    {
        const auto ponderMove = m_searchTables.getPonderMove();
        return ponderMove.isNull() ? std::nullopt : std::make_optional(ponderMove);
    }

    void resetNodes()
    {
        m_stackItr = m_stack.begin();
        m_nodes = 0;
        m_tbHits = 0;
        m_selDepth = 0;
        m_searchTables.resetHistoryNodes();
    }

    void reset()
    {
        m_searchTables.reset();
        m_repetition.reset();
        resetNodes();
    }

    void updateRepetition(uint64_t hash)
    {
        m_repetition.add(hash);
    }

    constexpr auto& getPvTable() const
    {
        return m_searchTables.getPvTable();
    }

    constexpr void printEvaluation(const BitBoard& board, std::optional<uint8_t> depthInput = std::nullopt)
    {
        resetNodes(); /* to reset nodes etc but not tables */

        uint8_t depth = depthInput.value_or(5);

        fmt::println("");

        movegen::ValidMoves captures;
        core::getAllMoves<movegen::MoveCapture>(board, captures);

        MovePicker<movegen::MoveCapture> capturePicker { m_searchTables, m_ply, PickerPhase::GenerateMoves };

        if (captures.count()) {
            fmt::print("Captures[{}]: ", captures.count());

            while (const auto& moveOpt = capturePicker.pickNextMove(board)) {
                const auto move = moveOpt.value();
                fmt::print("{}   ", move);
            }
            fmt::print("\n\n");
        }

        TimeManager::startInfinite();

        movegen::ValidMoves moves;
        core::getAllMoves<movegen::MovePseudoLegal>(board, moves);

        const Score score = negamax<true, true>(depth, board);

        TimeManager::stop();

        fmt::println("Move evaluations [{}]:", depth);

        MovePicker<movegen::MovePseudoLegal> allMovesPicker { m_searchTables, m_ply, PickerPhase::GenerateMoves };

        while (const auto& moveOpt = allMovesPicker.pickNextMove(board)) {
            const auto move = moveOpt.value();

            fmt::println("  {}", move);
        }

        fmt::println("\nTotal nodes:     {}\n"
                     "Search score:    {}\n"
                     "PV-line:         {}\n"
                     "Static eval:     {}\n",
            m_nodes, score, fmt::join(m_searchTables.getPvTable(), " "), m_staticEval.get(board));
    }

    template<bool isPv, bool isRoot = false>
    constexpr Score negamax(uint8_t depth, const BitBoard& board, Score alpha = s_minScore, Score beta = s_maxScore, bool cutNode = false, bool nullSearch = false)
    {
        m_searchTables.updatePvLength(m_ply);

        if constexpr (!isRoot) {
            const auto drawScore = checkForDraw(board);
            if (drawScore.has_value()) {
                return drawScore.value();
            }
        }

        const auto ttProbe = core::TranspositionTable::probe(m_stackItr->board.hash);
        if constexpr (!isPv && !isRoot) {
            if (ttProbe.has_value()) {
                const auto testResult = core::testEntry(*ttProbe, m_ply, depth, alpha, beta);
                if (testResult.has_value()) {
                    return testResult.value();
                }
            }
        }

        // Engine is not designed to search deeper than this! Make sure to stop before it's too late
        if (m_ply >= s_maxSearchDepth) {
            return m_staticEval.get(board);
        }

        const bool isChecked = core::isKingAttacked(board);
        if (isChecked) {
            /* Dangerous position - increase search depth
             * NOTE: there's rarely many legal moves in this position
             * so it's quite cheap to extend the search a bit */
            depth++;
        }

        if (depth == 0) {
            return quiesence<isPv>(board, alpha, beta);
        }

        m_nodes++;

        /* is the position part of the current or a previous PV line? */
        const bool ttPv = isPv || (ttProbe.has_value() && ttProbe->info.pv());

        Score correction = 0;
        if (isChecked) {
            /* evaluation terms are not considering king being attacked */
            m_stackItr->eval = -s_mateValue + m_ply;
        } else {
            correction = m_searchTables.getCorrectionHistory(board);
            /* update current stack with the static evaluation */
            m_stackItr->eval = fetchOrStoreEval(board, ttProbe, ttPv) + correction;
        }

        /* improving heuristics -> have the position improved since our last position? */
        const bool isImproving = !isChecked && m_ply >= 2 && (m_stackItr - 2)->eval < m_stackItr->eval;

        /* Max moves allowed by late move reduction */
        const uint64_t lmpMaxMoves = (spsa::lmpBase + spsa::lmpMargin * depth * depth) / (1 + spsa::lmpImproving * !isImproving);

        /* static pruning - try to prove that the position is good enough to not need
         * searching the entire branch */
        if constexpr (!isPv) {
            if (!isChecked) {
                /* https://www.chessprogramming.org/Reverse_Futility_Pruning */
                if (depth < spsa::rfpReductionLimit) {
                    const bool withinFutilityMargin = abs(beta - 1) > (s_minScore + spsa::rfpMargin);
                    const Score evalMargin = spsa::rfpEvaluationMargin * depth;

                    if (withinFutilityMargin && (m_stackItr->eval - evalMargin) >= beta)
                        return m_stackItr->eval - evalMargin;
                }

                /* dangerous to repeat null search on a null search - skip it here */
                if (!nullSearch) {
                    const Score nmpMargin = spsa::nmpBaseMargin + spsa::nmpMarginFactor * depth;
                    if (m_stackItr->eval + nmpMargin >= beta && !isRoot && !board.hasZugzwangProneMaterial()) {
                        if (const auto nullMoveScore = nullMovePruning(board, depth, beta, cutNode)) {
                            return nullMoveScore.value();
                        }
                    }
                }

                /* https://www.chessprogramming.org/Razoring (Strelka) */
                if (depth <= spsa::razorReductionLimit) {
                    Score score = m_stackItr->eval + spsa::razorMarginShallow;
                    if (score < beta) {
                        if (depth == 1) {
                            Score newScore = quiesence<isPv>(board, alpha, beta);
                            return (newScore > score) ? newScore : score;
                        }

                        score += spsa::razorMarginDeep;
                        if (score < beta && depth <= spsa::razorDeepReductionLimit) {
                            const Score newScore = quiesence<isPv>(board, alpha, beta);
                            if (newScore < beta)
                                return (newScore > score) ? newScore : score;
                        }

                        score += spsa::razorMarginDeep;
                        if (score < beta && depth <= spsa::razorDeepReductionLimit) {
                            const Score newScore = quiesence<isPv>(board, alpha, beta);
                            if (newScore < beta)
                                return (newScore > score) ? newScore : score;
                        }
                    }
                }
            }
        }

        const auto ttMove = tryFetchTtMove(ttProbe);

        /* internal iterative reduction (IIR)
         * the assumtion is that if no tt move was found for a given node
         * then it most not be very important - instead reduce it for now and let
         * future deeper searches explore that node (if important) */
        if (depth >= spsa::iirDepthLimit && (isPv || cutNode) && !ttMove.has_value()) {
            depth--;
        }

        auto phase = PickerPhase::GenerateMoves;

        if (syzygy::isTableActive(board)) {
            /* generateSyzygyMoves is not thread safe - allow primary searcher only to take this path! */
            if (isRoot && m_isPrimary) {
                phase = PickerPhase::GenerateSyzygyMoves;
            } else if (!isRoot) {
                const auto wdl = syzygy::probeWdl(board);
                if (wdl != syzygy::WdlResultFailed) {
                    m_tbHits++;
                    const auto wdlScore = syzygy::wdlToScore(wdl, m_ply);
                    const auto wdlTtFlag = syzygy::wdlToTtFlag(wdl);

                    /* similar to Transposition::testEntry - check if table will cause a cutoff and return early if so */
                    if (wdlTtFlag == core::TtExact
                        || (wdlTtFlag == core::TtAlpha && wdlScore <= alpha)
                        || (wdlTtFlag == core::TtBeta && wdlScore >= beta)) {
                        core::TranspositionTable::writeEntry(m_stackItr->board.hash, wdlScore, s_noScore, movegen::nullMove(), ttPv, depth, m_ply, wdlTtFlag);
                        return wdlScore;
                    }

                    /* syzygy already tells us the WDL outcome, but in PV nodes we continue
                     * the search to build a meaningful PV line — we still tighten alpha/beta
                     * to help guide pruning and avoid wasting effort */
                    if (isPv && wdlTtFlag == core::TtBeta) {
                        alpha = std::max(alpha, wdlScore);
                    } else if (isPv && wdlTtFlag == core::TtAlpha) {
                        beta = std::min(beta, wdlScore);
                    }
                }
            }
        }

        /* entries for the TT */
        core::TtFlag ttFlag = core::TtAlpha;
        movegen::Move bestMove {};
        Score bestScore = s_minScore;
        uint64_t movesSearched = 0;

        const auto prevMove = isRoot ? std::nullopt : std::make_optional((m_stackItr - 1)->move);
        MovePicker<movegen::MovePseudoLegal> picker(m_searchTables, m_ply, phase, ttMove, prevMove);

        while (const auto moveOpt = picker.pickNextMove(board)) {
            const auto move = moveOpt.value();

            /* static forward pruning
             * prune entire branches at pre-frontier nodes based on static criterias */
            if constexpr (!isPv) {
                if (!isChecked && !picker.getSkipQuiets() && bestScore > s_minScore) {

                    const uint8_t lmrDepth = depth - std::min<uint8_t>(getLmrReduction(depth, movesSearched), depth);
                    const Score margin = spsa::efpBase + (spsa::efpMargin * lmrDepth) + (spsa::efpImproving * isImproving);

                    /* extended futility pruning (EFP)
                     * after searching a certain depth we can start pruning
                     * moves that will most likely not improve alpha (for now skip quiets) */
                    if (lmrDepth <= spsa::efpDepthLimit && m_stackItr->eval + margin < alpha) {
                        picker.setSkipQuiets(true);
                    }

                    /* Late move pruning: https://www.madchess.net/2020/02/08/madchess-3-0-beta-4478cb8-late-move-pruning/ */
                    if (depth <= spsa::lmpDepthLimit && movesSearched >= lmpMaxMoves) {
                        picker.setSkipQuiets(true);
                    }
                }
            }

            if (!makeMove(board, move)) {
                continue;
            }

            Score score = 0;
            const uint64_t prevNodes = m_nodes;

            if (movesSearched == 0) {
                /* no moves searched yet -> perform a full depth PV move search */
                score = -negamax<isPv>(depth - 1, m_stackItr->board, -beta, -alpha, !(isPv || cutNode));
            } else {
                /* other moves we can attempt searched with a reduced zero window search
                 * if the zero window search increases alpha we increase the window size */
                int8_t reduction = 0;
                if (movesSearched >= spsa::fullDepthMove
                    && !move.isCapture()
                    && !move.isPromotionMove()) {

                    const bool isGivingCheck = core::isKingAttacked(m_stackItr->board);
                    reduction = getLmrReduction(depth, movesSearched);

                    reduction -= static_cast<int8_t>(isChecked); /* reduce less when checked */
                    reduction -= static_cast<int8_t>(isGivingCheck); /* reduce less when giving check */
                    reduction += static_cast<int8_t>(!isPv); /* reduce more when not pv line */
                    reduction += static_cast<int8_t>(!isImproving); /* reduce more when not improving */
                    reduction += static_cast<int8_t>(cutNode); /* reduce more when cut-node */

                    reduction = std::clamp<uint8_t>(reduction, 0, depth - 1);
                }

                /* first try zero window search with reduced depth (best case scenario) */
                score = -zeroWindow(depth - 1 - reduction, m_stackItr->board, -alpha, true);

                /* above failed high, so attempt a zero window search but with no reductions */
                if (score > alpha && reduction > 0) {
                    score = -zeroWindow(depth - 1, m_stackItr->board, -alpha, !cutNode);
                }

                /* if both reduced and non-reduced zero search failed high then we're forced
                 * to do a full depth full window search
                 * this search has PV potential, so the cost is worth it! */
                if (score > alpha && score < beta) {
                    score = -negamax<isPv>(depth - 1, m_stackItr->board, -beta, -alpha, !(isPv || cutNode));
                }
            }

            undoMove();

            if (isSearchStopped())
                return s_minScore;

            movesSearched++;

            if (isRoot) {
                m_searchTables.addHistoryNodes(move, m_nodes - prevNodes);
            }

            if (score > bestScore) {
                bestScore = score;
            }

            if (score > alpha) {
                alpha = score;

                ttFlag = core::TtExact;
                bestMove = move;

                m_searchTables.updateHistoryMoves(board, move, m_ply);
                m_searchTables.updatePvTable(move, m_ply);
            }

            if (score >= beta) {
                bestMove = move;
                ttFlag = core::TtBeta;

                m_searchTables.updateKillerMoves(move, m_ply);
                if constexpr (!isRoot) {
                    const auto prevMove = (m_stackItr - 1)->move;
                    m_searchTables.updateCounterMoves(prevMove, move);
                }

                break;
            }
        }

        if (movesSearched == 0) {
            if (isChecked) {
                // We want absolute negative score - but with amount of moves to the given checkmate
                // we add the ply to make checkmate in less moves a better move
                return -s_mateValue + m_ply;
            } else {
                // Stalemate - absolute neutral score
                return 0;
            }
        }

        /* update correction history only if all the following conditions are met:
         *   - the side to move is not in check  → avoids "unstable" positions
         *   - the best move is quiet            → excludes tactical/noisy positions
         *   - not an alpha cutoff               → avoids biased updates from fail-high
         *   - not a beta cutoff                 → avoids biased updates from fail-low
         *
         * this ensures only stable, meaningful positions contribute to correction history */
        if (!isChecked
            && bestMove.isQuietMove()
            && !(ttFlag == core::TtFlag::TtAlpha && bestScore >= m_stackItr->eval)
            && !(ttFlag == core::TtFlag::TtBeta && bestScore <= m_stackItr->eval)) {
            m_searchTables.updateCorrectionHistory(board, depth, bestScore, m_stackItr->eval);
        }

        core::TranspositionTable::writeEntry(m_stackItr->board.hash, bestScore, m_stackItr->eval - correction, bestMove, ttPv, depth, m_ply, ttFlag);
        return bestScore;
    }

private:
    inline Score zeroWindow(uint8_t depth, const BitBoard& board, Score window, bool cutNode, bool nullSearch = false)
    {
        return negamax<false>(depth, board, window - 1, window, cutNode, nullSearch);
    }

    template<bool isPv>
    constexpr Score quiesence(const BitBoard& board, Score alpha, Score beta)
    {
        m_nodes++;
        m_selDepth = std::max(m_selDepth, m_ply);

        const auto drawScore = checkForDraw(board);
        if (drawScore.has_value()) {
            return drawScore.value();
        }

        if (m_ply >= s_maxSearchDepth)
            return m_staticEval.get(board);

        const auto ttProbe = core::TranspositionTable::probe(m_stackItr->board.hash);
        const bool isChecked = core::isKingAttacked(board);
        const bool ttPv = isPv || (ttProbe.has_value() && ttProbe->info.pv());

        Score correction = 0;
        if (isChecked) {
            /* be careful to cause cutoffs when checked */
            m_stackItr->eval = -s_mateValue + m_ply;
        } else {
            correction = m_searchTables.getCorrectionHistory(board);
            /* update current stack with the static evaluation */
            m_stackItr->eval = fetchOrStoreEval(board, ttProbe, ttPv) + correction;
        }

        /* stand pat */
        if (m_stackItr->eval >= beta) {
            return m_stackItr->eval;
        }

        if (m_stackItr->eval > alpha) {
            alpha = m_stackItr->eval;
        }

        if (ttProbe.has_value()) {
            /* try to return early if we already "know the result" of the current qsearch
             * most of the time our score is based on the final qsearch, so this should give the
             * same result as searching the full line */
            const auto testResult = core::testEntry(*ttProbe, m_ply, 0, alpha, beta);
            if (testResult.has_value()) {
                return testResult.value();
            }
        }

        /* entries for the TT */
        core::TtFlag ttFlag = core::TtAlpha;
        movegen::Move bestMove = movegen::nullMove();
        Score bestScore = m_stackItr->eval;
        uint16_t movesSearched = 0;

        const auto ttMove = tryFetchTtMove(ttProbe);

        MovePicker<movegen::MoveCapture> picker { m_searchTables, m_ply, PickerPhase::GenerateMoves, ttMove };

        while (const auto& moveOpt = picker.pickNextMove(board)) {
            const auto move = moveOpt.value();

            if (!makeMove(board, move)) {
                continue;
            }

            const Score score = -quiesence<isPv>(m_stackItr->board, -beta, -alpha);
            undoMove();

            if (isSearchStopped())
                return s_minScore;

            movesSearched++;
            if (score > bestScore) {
                bestScore = score;
            }

            if (score >= beta) {
                bestMove = move;
                ttFlag = core::TtBeta;
                break;
            }

            if (score > alpha) {
                bestMove = move;
                ttFlag = core::TtExact;
                alpha = score;
            }
        }

        /* no captures - simply return static eval */
        if (picker.numGeneratedMoves() == 0) {
            return m_stackItr->eval;
        }

        /* no legal moves while being in check -> this must be a checkmate! */
        if (isChecked && movesSearched == 0) {
            return -s_mateValue + m_ply;
        }

        core::TranspositionTable::writeEntry(m_stackItr->board.hash, bestScore, m_stackItr->eval, bestMove, ttPv, 0, m_ply, ttFlag);
        return bestScore;
    }

    /*
     * null move pruning
     * https://www.chessprogramming.org/Null_Move_Pruning
     * */
    std::optional<Score> nullMovePruning(const BitBoard& board, uint8_t depth, Score beta, bool cutNode)
    {
        auto nullMoveBoard = board;

        /* add current position as potential repetition */
        m_repetition.add(board.hash);

        if (nullMoveBoard.enPessant.has_value())
            core::hashEnpessant(nullMoveBoard.enPessant.value(), nullMoveBoard.hash);

        core::hashPlayer(nullMoveBoard.hash);

        /* enPessant is invalid if we skip move */
        nullMoveBoard.enPessant.reset();

        /* give opponent an extra move */
        nullMoveBoard.player = nextPlayer(nullMoveBoard.player);

        m_stackItr += 2;
        m_stackItr->board = nullMoveBoard;
        m_stackItr->move = movegen::nullMove();

        m_ply += 2;

        /* perform search with reduced depth (based on reduction limit) */
        const uint8_t reduction = std::min<uint8_t>(depth, spsa::nmpReductionBase + depth / spsa::nmpReductionFactor);
        Score score = -zeroWindow(depth - reduction, nullMoveBoard, -beta + 1, !cutNode, true);

        m_ply -= 2;
        m_stackItr -= 2;
        m_repetition.remove();

        if (score >= beta) {
            return beta;
        }

        return std::nullopt;
    }

    bool makeMove(const BitBoard& board, movegen::Move move)
    {
        auto newBoard = core::performMove(board, move);
        if (core::isKingAttacked(newBoard, board.player)) {
            /* invalid move */
            return false;
        }

        /* add current position as potential repetition */
        m_repetition.add(board.hash);

        /* update stack for next iteration */
        m_stackItr++;
        m_stackItr->board = std::move(newBoard);
        m_stackItr->move = move;

        m_ply++;

        return true;
    }

    void undoMove()
    {
        m_stackItr--;
        m_repetition.remove();
        m_ply--;
    }

    /* fetch a potential TT move from a potential TT entry */
    inline std::optional<movegen::Move> tryFetchTtMove(std::optional<core::TtEntryData> entry)
    {
        if (!entry.has_value()) {
            return std::nullopt;
        }

        return entry->move.isNull() ? std::nullopt : std::make_optional(entry->move);
    }

    /* try to fetch the static evaluation from the TT entry, if any
     * otherwise compute the static evaluation based on the current board
     * position - and then try to update the TT with that evaluation  */
    inline Score fetchOrStoreEval(const BitBoard& board, std::optional<core::TtEntryData> entry, bool ttPv)
    {
        if (entry.has_value() && entry->eval != s_noScore) {
            return entry->eval;
        } else {
            const Score eval = m_staticEval.get(board);
            core::TranspositionTable::writeEntry(m_stackItr->board.hash, s_noScore, eval, movegen::nullMove(), ttPv, 0, m_ply, core::TtAlpha);
            return eval;
        }
    }

    inline bool isSearchStopped() const
    {
        if (s_searchStopped.load(std::memory_order_relaxed))
            return true;

        if (m_isPrimary && m_nodes % 2048 == 0) {
            TimeManager::updateTimeout();
        }

        return TimeManager::hasTimedOut();
    }

    inline std::optional<Score> checkForDraw(const BitBoard& board)
    {
        const bool isDraw = board.halfMoves >= 100 || m_repetition.isRepetition(board, m_stackItr->board.hash, m_ply) || board.hasInsufficientMaterial();
        if (isDraw) {
            return m_staticEval.getDrawScore(m_nodes, m_ply);
        }

        return std::nullopt;
    }

    static inline uint8_t s_numSearchers {};
    static inline std::atomic_bool s_searchStopped { true };

    uint64_t m_nodes {};
    uint64_t m_tbHits {};
    uint8_t m_ply {};
    Repetition m_repetition;
    SearchTables m_searchTables {};
    uint8_t m_selDepth {};
    bool m_isPrimary { true };

    struct StackInfo {
        BitBoard board;
        movegen::Move move;
        Score eval;
    };

    std::array<StackInfo, s_maxSearchDepth> m_stack;
    decltype(m_stack)::iterator m_stackItr = m_stack.begin();

    std::promise<SearcherResult> m_searchPromise;
    std::future<SearcherResult> m_futureResult;

    evaluation::StaticEvaluation m_staticEval;
};
}
