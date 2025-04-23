#include <catch2/catch_test_macros.hpp>
#include <future>
#include <iostream>
#include <latch>

#define private public
#include "engine/thread_pool.h"

TEST_CASE("Test Thread Pool", "[ThreadPool]")
{
    SECTION("Test constructor thread size")
    {
        ThreadPool defaultPool {};
        REQUIRE(defaultPool.m_workers.size() == 1);

        ThreadPool sizedPool { 4 };
        REQUIRE(sizedPool.m_workers.size() == 4);
    }

    SECTION("Test submit - normal case")
    {
        ThreadPool pool { 10 };

        const size_t iterations = pool.m_jobs.size();
        REQUIRE(iterations == pool.m_workers.size() * pool.s_jobScalar);

        std::latch workDone(iterations);

        for (size_t i = 0; i < iterations; ++i) {
            const bool success = pool.submit([&workDone]() {
                workDone.count_down();
            });

            REQUIRE(success);
        }

        workDone.wait();
    }

    SECTION("Test submit - too many jobs")
    {
        /* low amount of threads - just to increase the risk of not submitting a job */
        ThreadPool pool { 2 };

        const size_t iterations = 1000;
        std::latch workDone(iterations);

        /* fast and simple way to test over flow - if we don't succeed on submitting jobs then
         * we manuallly decrement the latch - the latch count should still match!
         * std::latch will throw an error if we exceed the limit */
        for (size_t i = 0; i < iterations; ++i) {
            const bool success = pool.submit([&]() {
                workDone.count_down();
            });

            if (!success) {
                workDone.count_down();
            }
        }

        workDone.wait();
    }
}

