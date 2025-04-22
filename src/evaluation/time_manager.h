#pragma once

#include "bit_board.h"
#include "board_defs.h"
#include <atomic>

namespace evaluation {

class TimeManager {
public:
    static bool startInifite()
    {
        const bool wasStopped = m_isStopped.exchange(false);
        if (!wasStopped)
            return false;

        m_startTime = std::chrono::system_clock::now();
        m_endTime = TimePoint::max();

        return true;
    }

    static bool start(const BitBoard& board)
    {
        const bool wasStopped = m_isStopped.exchange(false);
        if (!wasStopped)
            return false;

        m_startTime = std::chrono::system_clock::now();
        updateEndTime(board, m_startTime);

        return true;
    }

    /* will continue the search, if running, and update the end time to be based on the current time
     * useful for eg. pondering */
    static bool continueIfRunning(const BitBoard& board)
    {
        if (!isRunning())
            return false;

        const auto now = std::chrono::system_clock::now();
        updateEndTime(board, now);

        return true;
    }

    static bool timeForAnotherSearch(const BitBoard& board)
    {
        if (m_isStopped)
            return false;

        /* always allow full scan on first move - will be good for the hash table :) */
        if (board.fullMoves == 0)
            return true;

        const auto now = std::chrono::system_clock::now();
        const auto timeLeft = m_endTime.load(std::memory_order_relaxed) - now;
        const auto timeSpent = now - m_startTime;

        /* factor is "little less than half" meaning that we give juuust about half the time we spent to search a new depth
         * might need tweaking - will do when game phases are implemented */
        const auto timeLimit = timeSpent / 1.9;

        if (timeLeft < timeLimit) {
            return false;
        }

        return true;
    }

    /* lightweight check for stop - should be called from engine loop */
    static bool checkStopCondition(bool forceUpdate = false)
    {
        const uint64_t checkCounter = m_checkStopCount.fetch_add(1, std::memory_order_relaxed);
        if (forceUpdate || checkCounter % 2048 == 0) {
            const auto timeNow = std::chrono::system_clock::now();
            if (timeNow >= m_endTime.load(std::memory_order_relaxed)) {
                m_isStopped = true;
            }
        }

        return m_isStopped;
    }

    static bool isRunning()
    {
        return !m_isStopped;
    }

    static auto timeSearchedMs()
    {
        const auto timeNow = std::chrono::system_clock::now();
        return duration_cast<std::chrono::milliseconds>(timeNow - m_startTime);
    }

    static void setWhiteTime(uint64_t time)
    {
        m_whiteTime = time;
    }

    static void setBlackTime(uint64_t time)
    {
        m_blackTime = time;
    }

    static void setMovesToGo(uint64_t moves)
    {
        m_movesToGo = moves;
    }

    static void setMoveTime(uint64_t time)
    {
        m_moveTime = time;
    }

    static void setWhiteMoveInc(uint64_t inc)
    {
        m_whiteMoveInc = inc;
    }

    static void setBlackMoveInc(uint64_t inc)
    {
        m_blackMoveInc = inc;
    }

    static void reset()
    {
        if (isRunning())
            return;

        m_whiteTime = 0;
        m_blackTime = 0;
        m_movesToGo = 0;
        m_moveTime.reset();
        m_whiteMoveInc = 0;
        m_blackMoveInc = 0;
        m_checkStopCount = 0;
    }

    static void stop()
    {
        m_isStopped = true;
    }

private:
    static void updateEndTime(const BitBoard& board, const TimePoint& start)
    {
        using namespace std::chrono;

        /* allow some extra time for processing etc */
        constexpr auto buffer = 50ms;

        const auto timeLeft = board.player == PlayerWhite ? milliseconds(m_whiteTime) : milliseconds(m_blackTime);
        const auto timeInc = board.player == PlayerWhite ? milliseconds(m_whiteMoveInc) : milliseconds(m_blackMoveInc);

        /* no time provided - search for as long as possible */
        if (timeLeft == 0ms && timeInc == 0ms) {
            m_endTime = TimePoint::max();
            return;
        }

        if (m_moveTime) {
            m_endTime = start + milliseconds(m_moveTime.value()) - buffer + timeInc;
        } else if (m_movesToGo) {
            const auto time = timeLeft / m_movesToGo;
            m_endTime = start + time - buffer + timeInc;
        } else {
            /* based on time used by grandmasters
             * FIXME: not very precise  (manually extracted data from plot) */
            constexpr auto movesLeft = std::to_array<uint8_t>(
                { 50, 49, 48, 47, 46, 45, 44, 43, 42,
                    41, 40, 39, 38, 37, 36, 35, 34, 33, 32,
                    31, 30, 29, 28, 27, 26, 25, 24, 23, 22,
                    21, 20, 19, 18, 17, 16, 15, 14, 13, 12,
                    11, 10, 10, 10, 10, 10, 9, 9, 9, 9,
                    9, 9, 9, 8, 8, 8, 8, 8, 8, 8,
                    7, 7, 7, 7, 7, 7, 8, 8, 8, 8,
                    8, 9, 9, 9, 9, 9, 9, 9, 9, 9,
                    10, 10, 10, 10, 10, 10, 10, 10, 10, 11,
                    11, 11, 11, 11, 11, 11, 11, 11, 11, 12 });

            const uint32_t moveIndex = std::min(board.fullMoves, static_cast<uint32_t>(movesLeft.size() - 1));
            const uint8_t movesRemaining = movesLeft[moveIndex];

            /* Allocate time proportionally */
            const auto time = timeLeft / movesRemaining;
            m_endTime = start + time - buffer + timeInc;
        }
    }

    static inline std::atomic_bool m_isStopped { true };
    static inline std::atomic_uint64_t m_checkStopCount;

    static inline TimePoint m_startTime;
    static inline std::atomic<TimePoint> m_endTime;

    static inline uint64_t m_whiteTime {};
    static inline uint64_t m_blackTime {};
    static inline uint64_t m_movesToGo {};
    static inline std::optional<uint64_t> m_moveTime {};
    static inline uint64_t m_whiteMoveInc {};
    static inline uint64_t m_blackMoveInc {};
};

}
