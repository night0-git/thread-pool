#include "utils.h"
#include <thread_pool/thread_pool.h>
#include <thread_pool/detail/deque.h>
#include <thread_pool/detail/task.h>

#include <thread>
#include <vector>
#include <atomic>
#include <chrono>

using tp::detail::TaskDeque;
using tp::detail::Task;

TEST_CASE("Stress: Single-item race between pop and multiple thieves", "[stress]") {
    constexpr int iterations = 10'000;
    constexpr int num_thieves = 4;

    TaskDeque deque(16);
    std::atomic<bool> stop { false };
    std::atomic<size_t> stolen_count { 0 };

    std::vector<std::thread> thieves;
    thieves.reserve(num_thieves);
    for (int i = 0; i < num_thieves; i++) {
        thieves.emplace_back([&] {
            while (!stop.load(std::memory_order_relaxed)) {
                if (auto task = deque.steal()) {
                    stolen_count.fetch_add(1, std::memory_order_relaxed);
                }
                std::this_thread::yield();
            }
        });
    }

    size_t popped_count = 0;
    for (int i = 0; i < iterations; i++) {
        deque.push(std::make_unique<Task>([]{}));
        if (auto task = deque.pop()) {
            popped_count++;
        }
    }

    stop.store(true, std::memory_order_relaxed);
    for (auto& t : thieves) {
        t.join();
    }

    while (deque.pop()) {
        popped_count++;
    }

    REQUIRE(popped_count + stolen_count.load(std::memory_order_relaxed) == iterations);
}

TEST_CASE("Stress: Dynamic buffer resize under continuous stealing", "[stress]") {
    constexpr size_t initial_cap = 4;
    constexpr size_t total_tasks = 20'000;
    constexpr int num_thieves = 4;

    TaskDeque deque(initial_cap);
    std::atomic<bool> done_pushing { false };
    std::atomic<size_t> tasks_claimed { 0 };

    std::vector<std::thread> thieves;
    thieves.reserve(num_thieves);
    for (int i = 0; i < num_thieves; i++) {
        thieves.emplace_back([&] {
            while (!done_pushing.load(std::memory_order_relaxed) || deque.capacity() > 0) {
                if (auto task = deque.steal()) {
                    tasks_claimed.fetch_add(1, std::memory_order_relaxed);
                } else if (done_pushing.load(std::memory_order_relaxed)) {
                    break;
                }
            }
        });
    }

    for (size_t i = 0; i < total_tasks; i++) {
        deque.push(std::make_unique<Task>([]{}));
    }
    done_pushing.store(true, std::memory_order_relaxed);

    for (auto& t : thieves) {
        t.join();
    }

    while (deque.pop()) {
        tasks_claimed.fetch_add(1, std::memory_order_relaxed);
    }

    REQUIRE(tasks_claimed.load(std::memory_order_relaxed) == total_tasks);
}

TEST_CASE("Stress: Rapid pool construction and teardown under load", "[stress]") {
    for (int r = 0; r < 50; ++r) {
        tp::ThreadPool pool(8);
        for (int i = 0; i < 100; i++) {
            pool.submit([] {
                std::this_thread::sleep_for(std::chrono::microseconds(20));
            });
        }
    }
}
