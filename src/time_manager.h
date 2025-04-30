#pragma once

#include "bit_board.h"
#include "board_defs.h"
#include "engine/thread_pool.h"
#include <cstdint>
#include <mutex>
#include <optional>

class TimeManager {
public:
    static inline bool timeForAnotherSearch(const BitBoard& board)
    {
        if (m_timeOut)
            return false;

        /* always allow full scan on first move - will be good for the hash table :) */
        if (board.fullMoves > 0) {
            const auto now = std::chrono::system_clock::now();
            const auto timeLeft = m_endTime - now;
            const auto timeSpent = now - m_startTime;

            /* factor is "little less than half" meaning that we give juuust about half the time we spent to search a new depth
             * might need tweaking - will do when game phases are implemented */
            const auto timeLimit = timeSpent / 1.9;

            if (timeLeft < timeLimit) {
                return false;
            }
        }

        return true;
    }

    static inline void start(const BitBoard& board, ThreadPool& pool)
    {
        setupTimeControls(board);
        m_timeOut = false;

        std::ignore = pool.submit([] {
            while (!m_timeOut.load(std::memory_order_relaxed)) {
                {
                    std::scoped_lock lock(m_timeHandleMutex);
                    if (m_timeOut.load(std::memory_order_relaxed))
                        return; /* stopped elsewhere */

                    if (std::chrono::system_clock::now() > m_endTime) {
                        m_timeOut = true;
                    }
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });
    }

    static inline void startInfinite()
    {
        m_startTime = std::chrono::system_clock::now();
        m_endTime = TimePoint::max();
        m_timeOut = false;
    }

    static inline bool hasTimedOut()
    {
        return m_timeOut;
    }

    static inline void stop()
    {
        m_timeOut = true;
    }

    static inline auto timeElapsedMs()
    {
        const auto now = std::chrono::system_clock::now();
        return duration_cast<std::chrono::milliseconds>(now - m_startTime);
    }

    static inline void reset()
    {
        m_whiteTime = 0;
        m_blackTime = 0;
        m_movesToGo = 0;
        m_moveTime.reset();
        m_whiteMoveInc = 0;
        m_blackMoveInc = 0;
        m_timeOut = false;
    }

    static inline void setWhiteTime(uint64_t time)
    {
        m_whiteTime = time;
    }

    static inline void setBlackTime(uint64_t time)
    {
        m_blackTime = time;
    }

    static inline void setMovesToGo(uint64_t moves)
    {
        m_movesToGo = moves;
    }

    static inline void setMoveTime(uint64_t time)
    {
        m_moveTime = time;
    }

    static inline void setWhiteMoveInc(uint64_t inc)
    {
        m_whiteMoveInc = inc;
    }

    static inline void setBlackMoveInc(uint64_t inc)
    {
        m_blackMoveInc = inc;
    }

private:
    static inline void setupTimeControls(const BitBoard& board)
    {
        std::scoped_lock lock(m_timeHandleMutex);

        using namespace std::chrono;

        /* allow some extra time for processing etc */
        constexpr auto buffer = 50ms;

        const auto timeLeft = board.player == PlayerWhite ? milliseconds(m_whiteTime) : milliseconds(m_blackTime);
        const auto timeInc = board.player == PlayerWhite ? milliseconds(m_whiteMoveInc) : milliseconds(m_blackMoveInc);

        if (m_moveTime) {
            m_endTime = m_startTime + milliseconds(m_moveTime.value()) - buffer + timeInc;
        } else if (m_movesToGo) {
            const auto time = timeLeft / m_movesToGo;
            m_endTime = m_startTime + time - buffer + timeInc;
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

            m_endTime = m_startTime + adjustedTime - buffer + timeInc;
        }
    }

    static inline uint64_t m_whiteTime {};
    static inline uint64_t m_blackTime {};
    static inline uint64_t m_movesToGo {};
    static inline std::optional<uint64_t> m_moveTime {};
    static inline uint64_t m_whiteMoveInc {};
    static inline uint64_t m_blackMoveInc {};

    static inline TimePoint m_startTime;
    static inline TimePoint m_endTime;

    static inline std::mutex m_timeHandleMutex;
    static inline std::atomic_bool m_timeOut;

    constexpr static inline uint32_t s_defaultAmountMoves { 75 };
};
