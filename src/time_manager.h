#pragma once

#include "bit_board.h"
#include "board_defs.h"
#include "movegen/move_types.h"
#include "spsa/parameters.h"

#include <atomic>
#include <cstdint>
#include <optional>

class TimeManager {
public:
    static inline bool timeForAnotherSearch(uint8_t depth)
    {
        /* always search at least one depth to ensure we have a PV move */
        if (depth <= 1) {
            return true;
        }

        if (hasTimedOut()) {
            return false;
        }

        const auto now = std::chrono::steady_clock::now();
        const auto totalAllocated = s_softTimeLimit - s_startTime;

        double scalingFactor = s_pvMoveStabilityFactor;
        scalingFactor *= s_pvNodeScaleFactor;

        /* score is usually unstable in early depths - only track stability when we expect
         * the score to be "stable" */
        if (depth >= 7) {
            scalingFactor *= s_pvScoreStabilityFactor;
        }

        const auto adjustedTimeBudget = totalAllocated * scalingFactor;
        const auto adjustedTimeLimit = s_startTime + adjustedTimeBudget;

        return now < adjustedTimeLimit;
    }

    static inline void start(const BitBoard& board)
    {
        s_startTime = std::chrono::steady_clock::now();
        setupTimeControls(board);
        s_timedOut = false;
    }

    static inline void startInfinite()
    {
        s_startTime = std::chrono::steady_clock::now();
        s_softTimeLimit = TimePoint::max();
        s_hardTimeLimit = TimePoint::max();
        s_timedOut = false;
    }

    static inline void updateTimeout()
    {
        const auto now = std::chrono::steady_clock::now();
        if (now >= s_hardTimeLimit) {
            s_timedOut = true;
        }
    }

    static inline bool hasTimedOut()
    {
        /* called from engine loop - so ensure lock free */
        return s_timedOut.load(std::memory_order_relaxed);
    }

    static inline void stop()
    {
        s_timedOut = true;
    }

    static inline auto timeElapsedMs()
    {
        const auto now = std::chrono::steady_clock::now();
        return duration_cast<std::chrono::milliseconds>(now - s_startTime);
    }

    static inline void reset()
    {
        s_whiteTime = 0;
        s_blackTime = 0;
        s_movesToGo = 0;
        s_moveTime.reset();
        s_whiteMoveInc = 0;
        s_blackMoveInc = 0;
        s_timedOut = false;

        s_previousPvMove.reset();
        s_previousPvScore.reset();
        s_pvMoveStability = 0;
        s_pvScoreStability = 0;
        s_previousPvNodes = 0;
        s_pvMoveStabilityFactor = 1.0;
        s_pvScoreStabilityFactor = 1.0;
        s_pvNodeScaleFactor = 1.0;
    }

    static inline void setWhiteTime(uint64_t time)
    {
        s_whiteTime = time;
    }

    static inline void setBlackTime(uint64_t time)
    {
        s_blackTime = time;
    }

    static inline void setMovesToGo(uint64_t moves)
    {
        s_movesToGo = moves;
    }

    static inline void setMoveTime(uint64_t time)
    {
        s_moveTime = time;
    }

    static inline void setWhiteMoveInc(uint64_t inc)
    {
        s_whiteMoveInc = inc;
    }

    static inline void setBlackMoveInc(uint64_t inc)
    {
        s_blackMoveInc = inc;
    }

    /* updates move/score/node stability counters and scaling factors
     *
     * - if pvMove remains the same across iterations, increment move stability
     *   -> higher move stability reduces the soft time limit
     *
     * - if pvScore stays within a margin of the previous score, increment score stability
     *   -> higher score stability also reduces the soft time limit
     *
     * - if pvNodes increases, compute a scaling factor based on the relative growth
     *   -> larger node growth = lower scale factor = more aggressive cutoff
     *
     * - standardized stability tables (from Stash) are used to compute scale multipliers
     *
     * - stores pvMove, pvScore, and pvNodes for use in the next iteration */
    static inline void updateMoveStability(movegen::Move pvMove, Score pvScore, uint64_t pvNodes)
    {
        if (s_previousPvMove.has_value() && pvMove == *s_previousPvMove) {
            s_pvMoveStability++;
        } else {
            s_pvMoveStability = 0;
        }

        if (s_previousPvScore.has_value()
            && pvScore >= (*s_previousPvScore - spsa::timeManScoreMargin)
            && pvScore <= (*s_previousPvScore + spsa::timeManScoreMargin)) {
            s_pvScoreStability++;
        } else {
            s_pvScoreStability = 0;
        }

        if (pvNodes > 0) {
            /* cast as doubles to preserve precision */
            const double prev = static_cast<double>(s_previousPvNodes);
            const double curr = static_cast<double>(pvNodes);

            const double scalingDelta = (curr - prev) / curr;
            const double baseFrac = spsa::timeManNodeFracBase / 100.0 - scalingDelta;

            s_pvNodeScaleFactor = baseFrac * (spsa::timeManNodeFracMultiplier / 100.0);
        } else {
            s_pvNodeScaleFactor = 1.0;
        }

        /* pretty much standardized tables used by several engines (originally from Stash) */
        constexpr auto pvMoveStabilityTable = std::to_array<double>({ 2.5, 1.2, 0.9, 0.8, 0.75 });
        constexpr auto pvScoreStabilityTable = std::to_array<double>({ 1.25, 1.15, 1.0, 0.94, 0.88 });

        /* update scaling factors */
        s_pvMoveStabilityFactor = pvMoveStabilityTable[std::min<uint8_t>(s_pvMoveStability, 4)];
        s_pvScoreStabilityFactor = pvScoreStabilityTable[std::min<uint8_t>(s_pvScoreStability, 4)];

        /* update for next check */
        s_previousPvMove = pvMove;
        s_previousPvScore = pvScore;
        s_previousPvNodes = pvNodes;
    }

private:
    static inline void setupTimeControls(const BitBoard& board)
    {
        using namespace std::chrono;

        /* allow some extra time for processing etc */
        constexpr auto buffer = 50ms;

        const auto timeLeft = (board.player == PlayerWhite ? milliseconds(s_whiteTime) : milliseconds(s_blackTime));
        const auto timeInc = board.player == PlayerWhite ? milliseconds(s_whiteMoveInc) : milliseconds(s_blackMoveInc);

        if (s_moveTime) {
            s_softTimeLimit = s_startTime + milliseconds(s_moveTime.value()) - buffer + timeInc;
            s_hardTimeLimit = s_softTimeLimit;
        } else if (s_movesToGo) {
            const auto time = timeLeft / s_movesToGo;
            s_softTimeLimit = s_startTime + time - buffer + timeInc;
            s_hardTimeLimit = s_softTimeLimit;
        } else if (timeLeft.count() == 0 && timeInc.count() == 0) {
            /* no time was specified - search until stopped */
            s_softTimeLimit = TimePoint::max();
            s_hardTimeLimit = s_softTimeLimit;
        } else {
            /*
             * The above are "fixed" time controls. In real life we have to manage time ourselves.
             * Our time management model is based on the write from Sam Roelants:
             * https://github.com/sroelants/simbelmyne/blob/main/docs/time-management.md
             */
            const auto baseTime = duration_cast<milliseconds>(timeLeft * spsa::timeManBaseFrac / 1000.0)
                + duration_cast<milliseconds>(timeInc * spsa::timeManIncFrac / 100.0);

            s_softTimeLimit = s_startTime + duration_cast<milliseconds>(spsa::timeManSoftFrac / 100.0 * baseTime) - buffer;
            s_hardTimeLimit = s_startTime + duration_cast<milliseconds>(spsa::timeManHardFrac / 100.0 * baseTime) - buffer;
        }
    }

    static inline uint64_t s_whiteTime {};
    static inline uint64_t s_blackTime {};
    static inline uint64_t s_movesToGo {};
    static inline std::optional<uint64_t> s_moveTime {};
    static inline uint64_t s_whiteMoveInc {};
    static inline uint64_t s_blackMoveInc {};

    static inline TimePoint s_startTime;
    static inline TimePoint s_softTimeLimit;
    static inline TimePoint s_hardTimeLimit;

    static inline std::atomic_bool s_timedOut;

    /* stability storage - so we can compare with previous iteration */
    static inline std::optional<movegen::Move> s_previousPvMove;
    static inline std::optional<Score> s_previousPvScore;
    static inline uint64_t s_previousPvNodes { 0 };

    /* counters to check for how long the given parameter has been stable - higher is better */
    static inline uint8_t s_pvMoveStability { 0 };
    static inline uint8_t s_pvScoreStability { 0 };

    /* multiplier for soft limit - adjusted based on stability */
    static inline double s_pvMoveStabilityFactor { 1.0 };
    static inline double s_pvScoreStabilityFactor { 1.0 };
    static inline double s_pvNodeScaleFactor { 1.0 };
};
