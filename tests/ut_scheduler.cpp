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

TEST_CASE("Submit tasks to scheduler") {
    Scheduler s(1);

    INFO("Submit non void function");
    auto fut1 = s.submit([]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        return 10;
    });

    std::optional<int> res = std::nullopt;

    std::this_thread::sleep_for(std::chrono::milliseconds(450));
    res = fut1.poll();
    REQUIRE(!res.has_value());

    std::this_thread::sleep_for(std::chrono::milliseconds(550));
    res = fut1.poll();
    REQUIRE(res.has_value());
    REQUIRE(res.value() == 10);
}

TEST_CASE("Benchmark task execution on multiple workers") {
    auto bench = [&](int num_workers) {
        Scheduler s(num_workers);
        std::latch finished { 100 };

        for (int i = 0; i < 100; i++) {
            s.submit([&finished] {
                long_computation();
                finished.count_down();
                return 1;
            });
        }

        finished.wait();
    };

    BENCHMARK("Execute 100 tasks with 1 workers") {
        return bench(1);
    };
    BENCHMARK("Execute 100 tasks with 2 workers") {
        return bench(2);
    };
    BENCHMARK("Execute 100 tasks with 4 workers") {
        return bench(4);
    };
    BENCHMARK("Execute 100 tasks with 8 workers") {
        return bench(8);
    };
}

TEST_CASE("Inspect worker task distribution") {
    auto inspect = [](int num_tasks) {
        std::map<std::thread::id, int> worker_task_counts;

        Scheduler s(8);
        std::mutex mutex;
        std::latch finished { num_tasks };

        for (int i = 0; i < num_tasks; ++i) {
            s.submit([&worker_task_counts, &mutex, &finished] {
                {
                    std::lock_guard lock(mutex);
                    ++worker_task_counts[std::this_thread::get_id()];
                }

                long_computation();
                finished.count_down();
                return 1;
            });
        }

        finished.wait();

        std::lock_guard lk(mutex);
        INFO("Check active worker count");
        CHECK(worker_task_counts.size() == 8);
        std::println("Total tasks: {}", num_tasks);
        int idx = 0;
        for (const auto& [thread_id, count] : worker_task_counts) {
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