#pragma once

#include "bit_board.h"
#include "board_defs.h"
#include <atomic>
#include <cstdint>
#include <optional>

class TimeManager {
public:
    static inline bool timeForAnotherSearch()
    {
        if (hasTimedOut())
            return false;

        const auto now = std::chrono::steady_clock::now();
        return now < s_softTimeLimit;
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

private:
    static inline void setupTimeControls(const BitBoard& board)
    {
        using namespace std::chrono;

        /* allow some extra time for processing etc */
        constexpr auto buffer = 50ms;

        const auto timeLeft = board.player == PlayerWhite ? milliseconds(s_whiteTime) : milliseconds(s_blackTime);
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

            const auto baseTime = timeLeft / 20 + 3 * timeInc / 4;
            s_softTimeLimit = s_startTime + baseTime / 2 - buffer;
            s_hardTimeLimit = s_startTime + 3 * baseTime - buffer;
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

    constexpr static inline uint32_t s_defaultAmountMoves { 75 };
};
