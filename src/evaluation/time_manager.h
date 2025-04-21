#pragma once

#include "bit_board.h"
#include "board_defs.h"
#include <mutex>
#include <thread>

namespace evaluation {

class TimeManager {
public:
    static void startInifite()
    {
        using namespace std::chrono;

        m_startTime = system_clock::now();
        m_endTime = std::numeric_limits<TimePoint>::max();

        startTimeHandler();
    }

    static void start(const BitBoard& board)
    {
        using namespace std::chrono;

        m_startTime = system_clock::now();

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
            constexpr uint8_t defaultAmountMoves = 75;

            /* Estimate remaining moves */
            uint32_t movesRemaining = std::clamp(defaultAmountMoves - board.fullMoves, openingMoves, lateGameMoves);

            /* Allocate time proportionally */
            const auto time = timeLeft / movesRemaining;

            /* Adjust based on game phase (early = slightly faster, late = slightly deeper) */
            const float phaseFactor = 1.0 + (static_cast<float>(board.fullMoves) / defaultAmountMoves) * 0.5;
            const auto adjustedTime = duration_cast<milliseconds>(time * phaseFactor);

            m_endTime = m_startTime + adjustedTime - buffer + timeInc;
        }

        /* just to make sure that we actually search something - better to run out of time than to not search any moves.. */
        if (m_endTime <= m_startTime) {
            m_endTime = m_startTime + milliseconds(250) + timeInc;
        }

        startTimeHandler();
    }

    static bool isSearchDone(const BitBoard& board)
    {
        if (m_isStopped)
            return true;

        /* always allow full scan on first move - will be good for the hash table :) */
        if (board.fullMoves == 0)
            return false;

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
            return true;
        }

        return false;
    }

    static bool isStopped()
    {
        return m_isStopped;
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
        m_whiteTime = 0;
        m_blackTime = 0;
        m_movesToGo = 0;
        m_moveTime.reset();
        m_whiteMoveInc = 0;
        m_blackMoveInc = 0;
        m_isStopped = false;
    }

    static void stop()
    {
        m_isStopped = true;
    }

private:
    static void startTimeHandler()
    {
        bool wasStopped = m_isStopped.exchange(false);
        if (!wasStopped) {
            return; /* nothing to do here */
        }

        std::thread timeHandler([] {
            while (!m_isStopped) {
                {
                    std::scoped_lock lock(m_timeHandleMutex);
                    if (m_isStopped)
                        return; /* stopped elsewhere */

                    if (std::chrono::system_clock::now() > m_endTime) {
                        m_isStopped = true;
                    }
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });

        timeHandler.detach();
    }

    static inline std::atomic_bool m_isStopped { true };
    static inline std::mutex m_timeHandleMutex;

    static inline TimePoint m_startTime;
    static inline TimePoint m_endTime;

    static inline uint64_t m_whiteTime {};
    static inline uint64_t m_blackTime {};
    static inline uint64_t m_movesToGo {};
    static inline std::optional<uint64_t> m_moveTime {};
    static inline uint64_t m_whiteMoveInc {};
    static inline uint64_t m_blackMoveInc {};
};

}
