#include "utils.h"
#include <chase_lev_deque.h>
#include <latch>
#include <thread>
#include <barrier>

using queue::Deque;

TEST_CASE("Deque fundamental behavior",
          "[deque][single-thread]") {
    Deque<int> deque(8);

    REQUIRE_FALSE(deque.pop().has_value());
    REQUIRE_FALSE(deque.steal().has_value());
    REQUIRE(deque.capacity() == 8);

    REQUIRE(deque.push(10));
    REQUIRE(deque.push(20));
    REQUIRE(deque.push(30));

    SECTION("Owner pops from bottom in LIFO order") {
        REQUIRE(deque.pop() == 30);
        REQUIRE(deque.pop() == 20);
        REQUIRE(deque.pop() == 10);

        REQUIRE_FALSE(deque.pop().has_value());
    }

    SECTION("Thief steals from top in FIFO order") {
        REQUIRE(deque.steal() == 10);
        REQUIRE(deque.steal() == 20);
        REQUIRE(deque.steal() == 30);

        REQUIRE_FALSE(deque.steal().has_value());
    }

    SECTION("Owner and thief use opposite ends") {
        REQUIRE(deque.pop() == 30);
        REQUIRE(deque.steal() == 10);
        REQUIRE(deque.pop() == 20);

        REQUIRE_FALSE(deque.steal().has_value());
    }
}

TEST_CASE("Only one thread claims the final item",
          "[deque][multi-thread]") {
    constexpr int iterations = 10'000;

    for (int iteration = 0; iteration < iterations; iteration++) {
        Deque<int> deque(2);

        std::optional<int> popped;
        std::optional<int> stolen;

        std::atomic<bool> push_succeeded { false };

        // Ensures the thief does not begin until push() is complete.
        std::latch item_pushed { 1 };

        // Releases owner and thief into pop()/steal()
        // at approximately the same time.
        std::barrier race_start { 2 };

        std::thread owner([&] {
            push_succeeded.store(
                deque.push(42),
                std::memory_order_relaxed
            );

            item_pushed.count_down();
            race_start.arrive_and_wait();

            popped = deque.pop();
        });

        std::thread thief([&] {
            item_pushed.wait();
            race_start.arrive_and_wait();

            stolen = deque.steal();
        });

        owner.join();
        thief.join();

        REQUIRE(push_succeeded.load(std::memory_order_relaxed));

        const int successes =
            static_cast<int>(popped.has_value()) +
            static_cast<int>(stolen.has_value());

        REQUIRE(successes == 1);

        if (popped) {
            CHECK(*popped == 42);
        }

        if (stolen) {
            CHECK(*stolen == 42);
        }

        CHECK_FALSE(deque.steal().has_value());
    }
}

TEST_CASE("Multiple thieves claim every item exactly once",
          "[deque][multi-thread]") {
    constexpr int item_count = 1'000;
    constexpr int thief_count = 8;

    Deque<int> deque(item_count);

    for (int value = 0; value < item_count; value++) {
        REQUIRE(deque.push(value));
    }

    std::vector<std::atomic<int>> claims(item_count);

    for (auto& count : claims) {
        count.store(0, std::memory_order_relaxed);
    }

    std::atomic<int> claimed_items { 0 };
    std::latch start { thief_count };

    std::vector<std::thread> thieves;
    thieves.reserve(thief_count);

    for (int i = 0; i < thief_count; i++) {
        thieves.emplace_back([&] {
            start.arrive_and_wait();

            while (claimed_items.load(std::memory_order_relaxed)
                   < item_count) {
                std::optional<int> value = deque.steal();

                if (!value) {
                    std::this_thread::yield();
                    continue;
                }

                claims[*value].fetch_add(
                    1,
                    std::memory_order_relaxed
                );

                claimed_items.fetch_add(
                    1,
                    std::memory_order_relaxed
                );
            }
        });
    }

    for (std::thread& thief : thieves) {
        thief.join();
    }

    REQUIRE(claimed_items.load() == item_count);
    REQUIRE_FALSE(deque.steal().has_value());

    for (int value = 0; value < item_count; value++) {
        REQUIRE(claims[value].load() == 1);
    }
}

TEST_CASE("Owner pop and concurrent steals consume each item once",
          "[deque][multi-thread]") {
    constexpr int item_count = 10'000;
    constexpr int thief_count = 7;

    Deque<int> deque(item_count);

    std::vector<std::atomic<int>> claims(item_count);

    for (auto& count : claims) {
        count.store(0, std::memory_order_relaxed);
    }

    std::atomic<int> claimed_items { 0 };
    std::atomic<bool> all_items_pushed { false };
    std::atomic<bool> push_failed { false };

    std::latch owner_ready { 1 };
    std::barrier start_consuming { thief_count + 1 };

    auto record_claim = [&](int value) {
        if (value < 0 || value >= item_count) {
            push_failed.store(true, std::memory_order_relaxed);
            return;
        }

        claims[value].fetch_add(
            1,
            std::memory_order_relaxed
        );

        claimed_items.fetch_add(
            1,
            std::memory_order_relaxed
        );
    };

    std::thread owner([&] {
        for (int value = 0; value < item_count; value++) {
            if (!deque.push(value)) {
                push_failed.store(true, std::memory_order_relaxed);
                break;
            }
        }

        all_items_pushed.store(true, std::memory_order_release);
        owner_ready.count_down();

        start_consuming.arrive_and_wait();

        while (claimed_items.load(std::memory_order_relaxed)
               < item_count) {
            std::optional<int> value = deque.pop();

            if (value) {
                record_claim(*value);
            } else {
                std::this_thread::yield();
            }
        }
    });

    owner_ready.wait();

    std::vector<std::thread> thieves;
    thieves.reserve(thief_count);

    for (int i = 0; i < thief_count; i++) {
        thieves.emplace_back([&] {
            start_consuming.arrive_and_wait();

            while (claimed_items.load(std::memory_order_relaxed)
                   < item_count) {
                std::optional<int> value = deque.steal();

                if (value) {
                    record_claim(*value);
                } else {
                    std::this_thread::yield();
                }
            }
        });
    }

    owner.join();

    for (std::thread& thief : thieves) {
        thief.join();
    }

    REQUIRE(all_items_pushed.load(std::memory_order_acquire));
    REQUIRE_FALSE(push_failed.load(std::memory_order_relaxed));

    REQUIRE(claimed_items.load(std::memory_order_relaxed)
            == item_count);

    REQUIRE_FALSE(deque.steal().has_value());

    for (int value = 0; value < item_count; value++) {
        REQUIRE(
            claims[value].load(std::memory_order_relaxed) == 1
        );
    }
}

TEST_CASE("Deque safely reuses ring slots under contention",
          "[deque][multi-thread][stress]") {
    constexpr int capacity = 4;
    constexpr int item_count = 100'000;
    constexpr int thief_count = 4;

    Deque<int> deque(capacity);

    std::vector<std::atomic<int>> claims(item_count);

    for (auto& count : claims) {
        count.store(0, std::memory_order_relaxed);
    }

    std::atomic<int> claimed_items { 0 };
    std::atomic<bool> producer_finished { false };
    std::atomic<bool> invalid_value { false };

    std::barrier start { thief_count + 1 };

    auto record_claim = [&](int value) {
        if (value < 0 || value >= item_count) {
            invalid_value.store(true, std::memory_order_relaxed);
            return;
        }

        claims[value].fetch_add(
            1,
            std::memory_order_relaxed
        );

        claimed_items.fetch_add(
            1,
            std::memory_order_relaxed
        );
    };

    std::thread owner([&] {
        start.arrive_and_wait();

        int next_value = 0;

        while (next_value < item_count) {
            if (deque.push(next_value)) {
                next_value++;
                continue;
            }

            // The bounded deque is full. The owner removes work
            // from its own end to make progress.
            if (std::optional<int> value = deque.pop()) {
                record_claim(*value);
            } else {
                std::this_thread::yield();
            }
        }

        producer_finished.store(true, std::memory_order_release);

        // Drain anything thieves have not taken.
        while (claimed_items.load(std::memory_order_relaxed)
               < item_count) {
            if (std::optional<int> value = deque.pop()) {
                record_claim(*value);
            } else {
                std::this_thread::yield();
            }
        }
    });

    std::vector<std::thread> thieves;
    thieves.reserve(thief_count);

    for (int i = 0; i < thief_count; i++) {
        thieves.emplace_back([&] {
            start.arrive_and_wait();

            while (claimed_items.load(std::memory_order_relaxed)
                   < item_count) {
                if (std::optional<int> value = deque.steal()) {
                    record_claim(*value);
                } else {
                    std::this_thread::yield();
                }
            }
        });
    }

    owner.join();

    for (std::thread& thief : thieves) {
        thief.join();
    }

    REQUIRE(producer_finished.load(std::memory_order_acquire));
    REQUIRE_FALSE(invalid_value.load(std::memory_order_relaxed));

    REQUIRE(
        claimed_items.load(std::memory_order_relaxed) == item_count
    );

    REQUIRE_FALSE(deque.steal().has_value());

    for (int value = 0; value < item_count; value++) {
        REQUIRE(
            claims[value].load(std::memory_order_relaxed) == 1
        );
    }
}