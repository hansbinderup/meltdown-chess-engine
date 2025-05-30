#pragma once

#include "core/bit_board.h"
#include "core/board_defs.h"
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

        const Duration timeSpent = std::chrono::steady_clock::now() - s_startTime;

        double scalingFactor = s_pvMoveStabilityFactor;
        scalingFactor *= s_pvNodeScaleFactor;

        /* score is usually unstable in early depths - only track stability when we expect
         * the score to be "stable" */
        if (depth >= 7) {
            scalingFactor *= s_pvScoreStabilityFactor;
        }

        const auto adjustedTimeLimit = s_softTimeLimit * scalingFactor;
        return timeSpent < adjustedTimeLimit;
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
        s_softTimeLimit = Duration::max();
        s_hardTimeLimit = Duration::max();
        s_timedOut = false;
    }

    static inline void updateTimeout()
    {
        const Duration timeSpent = std::chrono::steady_clock::now() - s_startTime;
        if (timeSpent >= s_hardTimeLimit) {
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
        s_whiteTime = Duration::zero();
        s_blackTime = Duration::zero();
        s_whiteMoveInc = Duration::zero();
        s_blackMoveInc = Duration::zero();
        s_movesToGo = 0;
        s_moveTime.reset();
        s_timedOut = false;

        s_previousPvMove.reset();
        s_previousPvScore.reset();
        s_pvMoveStability = 0;
        s_pvScoreStability = 0;
        s_pvMoveStabilityFactor = 1.0;
        s_pvScoreStabilityFactor = 1.0;
        s_pvNodeScaleFactor = 1.0;
    }

    static inline void setWhiteTime(uint64_t time)
    {
        s_whiteTime = std::chrono::milliseconds(time);
    }

    static inline void setBlackTime(uint64_t time)
    {
        s_blackTime = std::chrono::milliseconds(time);
    }

    static inline void setMovesToGo(uint64_t moves)
    {
        s_movesToGo = moves;
    }

    static inline void setMoveTime(uint64_t time)
    {
        s_moveTime = std::chrono::milliseconds(time);
    }

    static inline void setWhiteMoveInc(uint64_t inc)
    {
        s_whiteMoveInc = std::chrono::milliseconds(inc);
    }

    static inline void setBlackMoveInc(uint64_t inc)
    {
        s_blackMoveInc = std::chrono::milliseconds(inc);
    }

    static inline void setMoveOverhead(uint64_t moveOverhead)
    {
        s_moveOverhead = std::chrono::milliseconds(moveOverhead);
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
    static inline void updateMoveStability(movegen::Move pvMove, Score pvScore, double nodeFraction)
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

        const double nodeBaseFrac = spsa::timeManNodeFracBase / 100.0 - nodeFraction;
        s_pvNodeScaleFactor = nodeBaseFrac * spsa::timeManNodeFracMultiplier / 100.0;

        /* pretty much standardized tables used by several engines (originally from Stash) */
        constexpr auto pvMoveStabilityTable = std::to_array<double>({ 2.5, 1.2, 0.9, 0.8, 0.75 });
        constexpr auto pvScoreStabilityTable = std::to_array<double>({ 1.25, 1.15, 1.0, 0.94, 0.88 });

        /* update scaling factors */
        s_pvMoveStabilityFactor = pvMoveStabilityTable[std::min<uint8_t>(s_pvMoveStability, 4)];
        s_pvScoreStabilityFactor = pvScoreStabilityTable[std::min<uint8_t>(s_pvScoreStability, 4)];

        /* update for next check */
        s_previousPvMove = pvMove;
        s_previousPvScore = pvScore;
    }

private:
    static inline void setupTimeControls(const BitBoard& board)
    {
        using namespace std::chrono;

        const auto timeInput = board.player == PlayerWhite ? s_whiteTime : s_blackTime;
        const auto timeInc = board.player == PlayerWhite ? s_whiteMoveInc : s_blackMoveInc;
        const auto timeLeft = subtractOverhead(timeInput);

        if (s_moveTime) {
            /* FIXME: should we account for move overhead here? */
            s_softTimeLimit = s_moveTime.value() + timeInc;
            s_hardTimeLimit = s_softTimeLimit;
        } else if (timeInput.count() == 0 && timeInc.count() == 0) {
            /* no time was specified - search until stopped */
            s_softTimeLimit = Duration::max();
            s_hardTimeLimit = s_softTimeLimit;
        } else {
            /* The above are "fixed" time controls. In real life we have to manage time ourselves.
             * Our time management model is based on the write from Sam Roelants:
             * https://github.com/sroelants/simbelmyne/blob/main/docs/time-management.md */

            /* if no "movesToGo" is provided then try to estimate one using our base parameters */
            const auto baseTime = (s_movesToGo
                                          ? timeLeft / s_movesToGo
                                          : timeLeft * spsa::timeManBaseFrac / 1000.0)
                + timeInc * spsa::timeManIncFrac / 100.0;

            const auto limitTime = duration_cast<milliseconds>(timeLeft * spsa::timeManLimitFrac / 100.0);

            s_softTimeLimit = std::min(limitTime, duration_cast<milliseconds>(spsa::timeManSoftFrac / 100.0 * baseTime));
            s_hardTimeLimit = std::min(limitTime, duration_cast<milliseconds>(spsa::timeManHardFrac / 100.0 * baseTime));
        }
    }

    /* time manager operates in milliseconds so scale the duration for easier conversion */
    using Duration = std::chrono::duration<double, std::milli>;

    /* ensure that we don't overflow when subtracting overhead */
    constexpr static inline Duration subtractOverhead(const Duration& duration)
    {
        if (s_moveOverhead > duration) {
            return Duration::zero();
        } else {
            return duration - s_moveOverhead;
        }
    }

    static inline Duration s_whiteTime {};
    static inline Duration s_blackTime {};
    static inline uint64_t s_movesToGo {};
    static inline std::optional<Duration> s_moveTime {};
    static inline Duration s_whiteMoveInc {};
    static inline Duration s_blackMoveInc {};

    static inline TimePoint s_startTime;
    static inline Duration s_softTimeLimit;
    static inline Duration s_hardTimeLimit;

    static inline std::atomic_bool s_timedOut;

    /* stability storage - so we can compare with previous iteration */
    static inline std::optional<movegen::Move> s_previousPvMove;
    static inline std::optional<Score> s_previousPvScore;

    /* counters to check for how long the given parameter has been stable - higher is better */
    static inline uint8_t s_pvMoveStability { 0 };
    static inline uint8_t s_pvScoreStability { 0 };

    /* multiplier for soft limit - adjusted based on stability */
    static inline double s_pvMoveStabilityFactor { 1.0 };
    static inline double s_pvScoreStabilityFactor { 1.0 };
    static inline double s_pvNodeScaleFactor { 1.0 };

    /* allow some extra time for processing etc */
    static inline Duration s_moveOverhead { s_defaultMoveOverhead };
};
