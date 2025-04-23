#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <stop_token>
#include <thread>
#include <vector>

class ThreadPool {
public:
    using Job = std::function<void()>;

    explicit ThreadPool(size_t threadCount = 1)
    {
        resize(threadCount);
    }

    ~ThreadPool()
    {
        m_stop.request_stop();

        /* wake threads */
        cv.notify_all();

        /* std::jthread auto joins */
    }

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
        m_jobs.resize(newThreadCount * 2);
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

            job();
        }
    }

    std::vector<std::jthread> m_workers;
    std::vector<Job> m_jobs;
    std::size_t m_jobIndex { 0 };
    std::stop_source m_stop;

    std::mutex queueMutex;
    std::condition_variable_any cv;
};

