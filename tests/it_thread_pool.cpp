#include "utils.h"
#include "thread_pool.h"
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