#include "utils.h"
#include <thread_pool/detail/deque.h>
#include <thread_pool/detail/task.h>
#include "deque/ring_buffer.h"
#include <thread>
#include <mutex>
#include <vector>
#include <latch>

using deque::TaskDeque;
using task::TaskAction, task::Task;

TEST_CASE("Deque operations and resize", "[deque]") {
    for (int i = 0; i < 100; i++) {
        constexpr size_t capacity = 1000;
        constexpr size_t num_tasks = capacity + 1;
        TaskDeque deque(capacity);

        TaskAction foo_action = [] {
            int a = 5;
        };

        for (int i = 0; i < num_tasks; i++) {
            deque.push(std::make_unique<Task>(foo_action));
        }
        CHECK(deque.capacity() == capacity * 2);

        std::mutex mtx;
        std::vector<std::unique_ptr<Task>> claimed_tasks;

        std::latch latch(3);

        auto steal_tasks = [&](TaskDeque& deque) {
            latch.arrive_and_wait();
            while (auto task = deque.steal()) {
                std::lock_guard lock(mtx);
                claimed_tasks.push_back(std::move(task));
            }
        };

        std::thread t1(steal_tasks, std::ref(deque));
        std::thread t2(steal_tasks, std::ref(deque));

        latch.arrive_and_wait();
        while (auto task = deque.pop()) {
            std::lock_guard lock(mtx);
            claimed_tasks.push_back(std::move(task));
        }

        t1.join();
        t2.join();

        REQUIRE(claimed_tasks.size() == num_tasks);
        REQUIRE_FALSE(deque.pop());
    }
}

TEST_CASE("Deque old buffers are reclaimed", "[deque]") {
    constexpr size_t capacity = 5;
    constexpr size_t expected_resizes = 4;
    constexpr size_t num_tasks = capacity << expected_resizes;
    {
        TaskDeque deque(capacity);
        REQUIRE(num_instances == 1);

        TaskAction foo_action = [] {
            int a = 5;
        };

        for (int i = 0; i < num_tasks; i++) {
            deque.push(std::make_unique<Task>(foo_action));
        }

        REQUIRE(num_instances == expected_resizes + 1);
    }
    REQUIRE(num_instances == 0);
}