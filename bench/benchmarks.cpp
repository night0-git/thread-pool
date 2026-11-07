#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

#include <thread_pool/thread_pool.h>
#include "mutex_pool.h"

#include <vector>
#include <future>
#include <latch>
#include <cstdint>

namespace {

void busy_work(std::uint64_t iterations) {
    volatile std::uint64_t result = 0;
    for (std::uint64_t j = 0; j < iterations; ++j) {
        result += j * j;
    }
}

}

TEST_CASE("Benchmark: High-throughput independent tasks", "[benchmark][throughput]") {
    constexpr int num_tasks = 20'000;
    const unsigned int hw_threads = std::max(2u, std::thread::hardware_concurrency());

    BENCHMARK("Work-Stealing Pool (" + std::to_string(hw_threads) + " workers)") {
        tp::ThreadPool pool(hw_threads);
        std::latch finished { num_tasks };

        for (int i = 0; i < num_tasks; i++) {
            pool.submit([&finished] {
                finished.count_down();
            });
        }
        finished.wait();
    };

    BENCHMARK("Mutex Shared-Queue Pool (" + std::to_string(hw_threads) + " workers)") {
        MutexThreadPool pool(hw_threads);
        std::latch finished{num_tasks};

        for (int i = 0; i < num_tasks; i++) {
            pool.submit([&finished] {
                finished.count_down();
            });
        }
        finished.wait();
    };
}

TEST_CASE("Benchmark: Thread count scaling on CPU compute", "[benchmark][scaling]") {
    constexpr int num_tasks = 200;
    constexpr std::uint64_t work_iters = 500'000;

    for (int num_workers : {1, 2, 4, 8}) {
        BENCHMARK("Work-Stealing: " + std::to_string(num_workers) + " workers") {
            tp::ThreadPool pool(num_workers);
            std::latch finished{num_tasks};

            for (int i = 0; i < num_tasks; i++) {
                pool.submit([&finished] {
                    busy_work(work_iters);
                    finished.count_down();
                });
            }
            finished.wait();
        };

        BENCHMARK("Mutex Pool: " + std::to_string(num_workers) + " workers") {
            MutexThreadPool pool(num_workers);
            std::latch finished{num_tasks};

            for (int i = 0; i < num_tasks; i++) {
                pool.submit([&finished] {
                    busy_work(work_iters);
                    finished.count_down();
                });
            }
            finished.wait();
        };
    }
}

TEST_CASE("Benchmark: Comparison with std::async", "[benchmark][async]") {
    constexpr int num_tasks = 200;
    constexpr std::uint64_t work_iters = 100'000;
    const unsigned int hw_threads = std::max(2u, std::thread::hardware_concurrency());

    BENCHMARK("Work-Stealing Pool") {
        tp::ThreadPool pool(hw_threads);
        std::vector<tp::Future<void>> futures;
        futures.reserve(num_tasks);

        for (int i = 0; i < num_tasks; i++) {
            futures.push_back(pool.submit([] {
                busy_work(work_iters);
            }));
        }
        for (auto& f : futures) {
            f.get();
        }
    };

    BENCHMARK("std::async (std::launch::async)") {
        std::vector<std::future<void>> futures;
        futures.reserve(num_tasks);

        for (int i = 0; i < num_tasks; i++) {
            futures.push_back(std::async(std::launch::async, [] {
                busy_work(work_iters);
            }));
        }
        for (auto& f : futures) {
            f.get();
        }
    };
}
