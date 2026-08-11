#include "utils.h"
#include "scheduler.h"
#include <chrono>
#include <map>
#include <print>
#include <latch>

using scheduler::Scheduler;

void long_computation() {
    volatile std::uint64_t result = 0;

    for (std::uint64_t j = 0; j < 1'000'000; ++j) {
        result += j * j;
    }
}

TEST_CASE("Submit tasks to scheduler", "[scheduler]") {
    Scheduler s(1);

    Future<void> fut1 = s.submit([]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    });
    Future<int> fut2 = s.submit([]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        return 10;
    });

    REQUIRE_NOTHROW(fut1.get());
    REQUIRE(fut2.get() == 10);

    REQUIRE_THROWS(fut1.get());
    REQUIRE_THROWS(fut2.get());
}

TEST_CASE("Benchmark task execution on multiple workers", "[scheduler]") {
    auto bench = [&](int num_workers, int num_tasks) {
        Scheduler s(num_workers);
        std::latch finished { num_tasks };

        for (int i = 0; i < num_tasks; i++) {
            s.submit([&finished] {
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

TEST_CASE("Inspect worker task distribution", "[scheduler]") {
    auto inspect = [](int num_tasks) {
        std::map<std::thread::id, int> num_worker_tasks;

        Scheduler s(8);
        std::mutex mutex;
        std::latch finished { num_tasks };

        for (int i = 0; i < num_tasks; i++) {
            s.submit([&num_worker_tasks, &mutex, &finished] {
                {
                    std::lock_guard lock(mutex);
                    num_worker_tasks[std::this_thread::get_id()]++;
                }

                long_computation();
                finished.count_down();
                return 1;
            });
        }

        finished.wait();

        std::lock_guard lk(mutex);
        INFO("Check active worker count");
        CHECK(num_worker_tasks.size() == 8);
        std::println("Total tasks: {}", num_tasks);
        int idx = 0;
        for (const auto& [thread_id, count] : num_worker_tasks) {
            std::println("worker {}: {} tasks ({:.0f}%)",
                idx++, count,
                static_cast<double>(count) / num_tasks * 100.0);
        }
        std::println("");
    };
    inspect(100);
    inspect(1000);
    inspect(10000);
}