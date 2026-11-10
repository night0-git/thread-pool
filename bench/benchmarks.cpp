#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

#include <thread_pool/thread_pool.h>
#include "mutex_pool.h"

#include <vector>
#include <future>
#include <latch>
#include <cstdint>
#include <random>
#include <algorithm>

namespace {

void busy_work(std::uint64_t iterations) {
    volatile std::uint64_t result = 0;
    for (std::uint64_t j = 0; j < iterations; ++j) {
        result += j * j;
    }
}

std::vector<std::uint64_t> generate_non_uniform_workload(size_t num_tasks) {
    std::vector<std::uint64_t> workload;
    workload.reserve(num_tasks);

    // Deterministic PRNG with fixed seed for reproducibility across runs
    std::mt19937 rng(42);
    // Exponential distribution with mean ~5,000 iterations:
    // Creates a realistic heavy-tailed workload where most tasks are fast
    // and a minority of tasks are significantly heavier.
    std::exponential_distribution<double> dist(1.0 / 5000.0);

    for (size_t i = 0; i < num_tasks; ++i) {
        auto val = static_cast<std::uint64_t>(dist(rng));
        workload.push_back(std::max<std::uint64_t>(100, val));
    }
    return workload;
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
    constexpr int num_tasks = 10'000;
    constexpr std::uint64_t work_iters = 5'000;

    for (int num_workers : {1, 2, 4, 8}) {
        tp::ThreadPool pool(num_workers);
        BENCHMARK("Work-Stealing: " + std::to_string(num_workers) + " workers") {
            std::latch finished{num_tasks};

            for (int i = 0; i < num_tasks; i++) {
                pool.submit([&finished] {
                    busy_work(work_iters);
                    finished.count_down();
                });
            }
            finished.wait();
        };

        MutexThreadPool mutex_pool(num_workers);
        BENCHMARK("Mutex Pool: " + std::to_string(num_workers) + " workers") {
            std::latch finished{num_tasks};

            for (int i = 0; i < num_tasks; i++) {
                mutex_pool.submit([&finished] {
                    busy_work(work_iters);
                    finished.count_down();
                });
            }
            finished.wait();
        };
    }
}

TEST_CASE("Benchmark: Non-uniform tasks on CPU compute", "[benchmark][non_uniform]") {
    constexpr int num_tasks = 10'000;
    const auto& workload = generate_non_uniform_workload(num_tasks);

    for (int num_workers : {1, 2, 4, 8}) {
        tp::ThreadPool pool(num_workers);
        BENCHMARK("Work-Stealing: " + std::to_string(num_workers) + " workers") {
            std::latch finished{num_tasks};

            for (int i = 0; i < num_tasks; i++) {
                const std::uint64_t iters = workload[i];
                pool.submit([iters, &finished] {
                    busy_work(iters);
                    finished.count_down();
                });
            }
            finished.wait();
        };

        MutexThreadPool mutex_pool(num_workers);
        BENCHMARK("Mutex Pool: " + std::to_string(num_workers) + " workers") {
            std::latch finished{num_tasks};

            for (int i = 0; i < num_tasks; i++) {
                const std::uint64_t iters = workload[i];
                mutex_pool.submit([iters, &finished] {
                    busy_work(iters);
                    finished.count_down();
                });
            }
            finished.wait();
        };
    }
}

TEST_CASE("Benchmark: Comparison with std::async", "[benchmark][async]") {
    constexpr int num_tasks = 1'000;
    constexpr std::uint64_t work_iters = 10'000;
    const unsigned int hw_threads = std::max(2u, std::thread::hardware_concurrency());

    tp::ThreadPool pool(hw_threads);

    BENCHMARK("Work-Stealing Pool") {
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
