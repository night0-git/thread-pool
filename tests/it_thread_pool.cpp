#include "utils.h"
#include <thread_pool/thread_pool.h>
#include <chrono>
#include <latch>

using thread_pool::ThreadPool;

void long_computation() {
    volatile std::uint64_t result = 0;

    for (std::uint64_t j = 0; j < 1'000'000; ++j) {
        result += j * j;
    }
}

TEST_CASE("Submit tasks to thread pool", "[thread-pool]") {
    ThreadPool pool(1);

    Future<void> fut1 = pool.submit([]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    });
    Future<int> fut2 = pool.submit([]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        return 10;
    });

    REQUIRE_NOTHROW(fut1.get());
    REQUIRE(fut2.get() == 10);

    REQUIRE_THROWS(fut1.get());
    REQUIRE_THROWS(fut2.get());
}

TEST_CASE("Benchmark task execution on multiple workers", "[thread-pool]") {
    auto bench = [&](int num_workers, int num_tasks) {
        ThreadPool pool(num_workers);
        std::latch finished { num_tasks };

        for (int i = 0; i < num_tasks; i++) {
            pool.submit([&finished] {
                long_computation();
                finished.count_down();
                return 1;
            });
        }

        finished.wait();
    };

    SECTION("Execute 100 tasks") {
        BENCHMARK("Execute 100 tasks with 2 workers") {
            return bench(2, 100);
        };
        BENCHMARK("Execute 100 tasks with 4 workers") {
            return bench(4, 100);
        };
        BENCHMARK("Execute 100 tasks with 8 workers") {
            return bench(8, 100);
        };
        BENCHMARK("Execute 100 tasks with 16 workers") {
            return bench(16, 100);
        };
    }

    SECTION("Execute 1000 tasks") {
        BENCHMARK("Execute 1000 tasks with 4 workers") {
            return bench(4, 1000);
        };
        BENCHMARK("Execute 1000 tasks with 8 workers") {
            return bench(8, 1000);
        };
        BENCHMARK("Execute 1000 tasks with 16 workers") {
            return bench(16, 1000);
        };
    }

    SECTION("Execute 1000 tasks") {
        BENCHMARK("Execute 2000 tasks with 8 workers") {
            return bench(8, 1000);
        };
        BENCHMARK("Execute 2000 tasks with 16 workers") {
            return bench(16, 1000);
        };
    }
}

TEST_CASE("Workers wake correctly from scheduler state",
          "[thread-pool][atomic-state]") {
    constexpr size_t num_workers = 4;
    constexpr size_t num_rounds = 100;
    constexpr size_t tasks_per_round = 16;

    ThreadPool pool(num_workers);

    std::atomic<size_t> completed { 0 };

    for (size_t round = 0; round < num_rounds; round++) {
        std::vector<Future<void>> futures;
        futures.reserve(tasks_per_round);

        for (size_t i = 0; i < tasks_per_round; i++) {
            futures.push_back(pool.submit([&completed] {
                completed.fetch_add(1, std::memory_order_relaxed);
            }));
        }

        // Force every task in this round to complete before
        // submitting the next round. This repeatedly lets the
        // scheduler reach state == 0 and workers go back to wait().
        for (auto& future : futures) {
            REQUIRE_NOTHROW(future.get());
        }

        REQUIRE(
            completed.load(std::memory_order_relaxed)
            == (round + 1) * tasks_per_round
        );

        // Give idle workers an opportunity to enter atomic::wait()
        // before the next batch is submitted.
        std::this_thread::sleep_for(
            std::chrono::milliseconds(1)
        );
    }

    REQUIRE(
        completed.load(std::memory_order_relaxed)
        == num_rounds * tasks_per_round
    );
}

TEST_CASE("Scheduler executes every task exactly once",
          "[thread-pool][atomic-state]") {
    constexpr size_t num_workers = 8;
    constexpr size_t num_tasks = 10'000;

    ThreadPool pool(num_workers);

    std::atomic<size_t> completed{0};

    std::vector<Future<void>> futures;
    futures.reserve(num_tasks);

    for (size_t i = 0; i < num_tasks; i++) {
        futures.push_back(pool.submit([&completed] {
            completed.fetch_add(1, std::memory_order_relaxed);
        }));
    }

    for (auto& future : futures) {
        REQUIRE_NOTHROW(future.get());
    }

    REQUIRE(
        completed.load(std::memory_order_relaxed)
        == num_tasks
    );
}