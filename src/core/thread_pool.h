#pragma once

#include <cassert>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <stop_token>
#include <thread>
#include <vector>

/**
 * ThreadPool — simple, fast, LIFO job runner.
 *
 * This thread pool is built with LIFO (last-in-first-out) job ordering.
 * Why LIFO? Two reasons:
 *
 * 1. We don’t care about job order — especially in a Lazy SMP-style chess
 *    engine where search is already non-deterministic. So FIFO isn’t worth the cost.
 * 2. LIFO is slightly faster — better for cache locality and less bookkeeping.
 *
 * The job queue is a fixed-size stack, chosen for performance. If it fills up,
 * `submit()` will return false — it's up to the caller to check and handle that.
 * No dynamic allocation, no locks unless there's contention, and no futures.
 *
 * Limitations:
 * - Fixed-size buffer: if all slots are full, new jobs are rejected.
 * - No return values or error tracking — fire-and-forget only.
 * - All threads share a single LIFO queue — no per-thread queues or stealing.
 *
 * Good for fast, parallel workloads where you just want things to run quickly
 * and don't care about order or result tracking — like a search thread pool.
 *
 * And yes, this description is AI generated - with a bit of human help :)
 */

class ThreadPool {
public:
    using Job = std::function<void()>;

    explicit ThreadPool(size_t threadCount = 1)
    {
        assert(threadCount);
        resize(threadCount);
    }

    ~ThreadPool()
    {
        m_stop.request_stop();

        /* wake threads */
        cv.notify_all();

        /* std::jthread auto joins */
    }

    [[nodiscard("Job might not be posted - ensure to check!")]]
    bool submit(Job job)
    {
        {
            std::lock_guard lock(queueMutex);

            if (m_jobIndex >= m_jobs.size()) {
                return false;
            }

            m_jobs[m_jobIndex++] = std::move(job);
        }

        cv.notify_one();
        return true;
    }

    void resize(size_t newThreadCount)
    {
        if (m_workers.size() == newThreadCount)
            return;

        /* stop old threads */
        m_stop.request_stop();
        cv.notify_all();
        m_workers.clear();

        /* no need to join - threads auto join on destruction from above */

        /* reset the new pool */
        std::lock_guard lock(queueMutex);
        m_stop = std::stop_source();
        m_workers.reserve(newThreadCount);
        m_jobs.resize(newThreadCount * s_jobScalar);
        m_jobIndex = 0;

        for (size_t i = 0; i < newThreadCount; ++i) {
            m_workers.emplace_back(
                [this](std::stop_token stoken) { this->worker(stoken); },
                m_stop.get_token());
        }
    }

private:
    void worker(std::stop_token stopToken)
    {
        while (!stopToken.stop_requested()) {
            Job job;
            {
                std::unique_lock lock(queueMutex);
                cv.wait(lock, stopToken, [&] { return m_jobIndex > 0; });

                if (m_jobIndex == 0)
                    continue;

                job = std::move(m_jobs[--m_jobIndex]);
            }

            if (job)
                job();
        }
    }

    std::vector<std::jthread> m_workers;
    std::vector<Job> m_jobs;
    std::size_t m_jobIndex { 0 };
    std::stop_source m_stop;

    std::mutex queueMutex;
    std::condition_variable_any cv;

    /* queue does not expand so ensure that we have enough space to actually queue jobs */
    constexpr static inline size_t s_jobScalar { 2 };
};
