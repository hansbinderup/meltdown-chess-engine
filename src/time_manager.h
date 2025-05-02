#pragma once

#include "bit_board.h"
#include "board_defs.h"
#include <atomic>
#include <cstdint>
#include <optional>

class TimeManager {
public:
    static inline bool timeForAnotherSearch(const BitBoard& board)
    {
        if (s_timedOut)
            return false;

        /* always allow full scan on first move - will be good for the hash table :) */
        if (board.fullMoves > 0) {
            const auto now = std::chrono::steady_clock::now();
            const auto timeLeft = s_endTime - now;
            const auto timeSpent = now - s_startTime;

            /* factor is "little less than half" meaning that we give juuust about half the time we spent to search a new depth
             * might need tweaking - will do when game phases are implemented */
            const auto timeLimit = timeSpent / 1.9;

            if (timeLeft < timeLimit) {
                return false;
            }
        }

        return true;
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
        s_endTime = TimePoint::max();
        s_timedOut = false;
    }

    static inline void updateTimeout()
    {
        const auto now = std::chrono::steady_clock::now();
        if (now >= s_endTime) {
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
            s_endTime = s_startTime + milliseconds(s_moveTime.value()) - buffer + timeInc;
        } else if (s_movesToGo) {
            const auto time = timeLeft / s_movesToGo;
            s_endTime = s_startTime + time - buffer + timeInc;
        } else if (timeLeft.count() == 0 && timeInc.count() == 0) {
            /* no time was specified - search until stopped */
            s_endTime = TimePoint::max();
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

            s_endTime = s_startTime + adjustedTime - buffer + timeInc;
        }
    }

    static inline uint64_t s_whiteTime {};
    static inline uint64_t s_blackTime {};
    static inline uint64_t s_movesToGo {};
    static inline std::optional<uint64_t> s_moveTime {};
    static inline uint64_t s_whiteMoveInc {};
    static inline uint64_t s_blackMoveInc {};

    static inline TimePoint s_startTime;
    static inline TimePoint s_endTime;

    static inline std::atomic_bool s_timedOut;

    constexpr static inline uint32_t s_defaultAmountMoves { 75 };
};
