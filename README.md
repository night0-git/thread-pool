# Introduction

This is an *educational* work-stealing thread pool in C++ for me to learn more about concurrency primitives such as future, promise, threads, and also lock-free programming.

# Project overview

The interface provided is simply a `ThreadPool` for user to submit tasks to be executed on the background, and a `Future` type to wait for the result of the task.

The core mechanic of task allocation is that a thread will try to steal from other threads after it has exhausted its own pool. A lock-free single producer - multiple consumer (SPMC) deque is used to hold tasks, which was the hardest part to implement.

# Usage

### Example

```cpp
#include <iostream>
#include <thread_pool/thread_pool.h>

int main() {
    // Create a thread pool with 4 worker threads.
    tp::ThreadPool pool(4);

    // Submit a task that returns a value.
    tp::Future<int> fut1 = pool.submit([]() {
        return 42;
    });

    // Submit a void task.
    tp::Future<void> fut2 = pool.submit([]() {
        std::cout << "Task executed asynchronously\n";
    });

    // Wait for the results.
    std::cout << "Result: " << fut1.get() << '\n';
    fut2.get();

    return 0;
}
```

### Installation

The library could be installed via CMake:
```CMake
find_package(thread-pool REQUIRED)

add_executable(<your executable> <source files>)
target_link_libraries(<your executable> PRIVATE thread_pool::thread_pool)
```
Or you could use FetchContent:
```CMake
include(FetchContent)

FetchContent_Declare(
    thread_pool
    GIT_REPOSITORY https://github.com/night0-git/thread-pool.git
)
FetchContent_MakeAvailable(thread_pool)

add_executable(<your executable> <source files>)
target_link_libraries(<your executable> PRIVATE thread_pool::thread_pool)
```

# Implementation details

### 1. Custom future

A simplified version of the standard library's `future` and `promise` is implemented for this project: `Future` and `Promise`.

How they work (together): A `Future` and a `Promise` share a unified `State` which both can read and write to. `Future` is held by the consumer and it contains method to poll the result, while a `Promise` is what the producer uses to "complete" its corresponding `Future`. In the code, a conditional variable is used by the `Future` to block the thread while waiting for `Promise` to complete it (during poll).

A future is the result of a submitted task, so it may not contain a value (void). In C++ this is handled through 'template specialization' for `void` type.

### 2. Lock-free SPMC deque

Also called a Chase-Lev deque, it is a double-ended queue implemented purely with atomic primitives, which makes it completely thread-safe without a mutex.

A deque provides a `pop`/`push` method that only the owner thread can use to take tasks from the bottom, and a `steal` method that any thread (thief) can use to steal tasks from the top. This separation means that the owner and thieves normally operate on different ends of the deque; the only case where they may contend for the same task is when exactly one task remains, which can be resolved by a CAS.

The implementation is highly formulated, revolving around the 'memory ordering' property of the CPU.

### 3. Scheduler and thread pool

A `Scheduler` basically contains a number of 'workers', each of which corresponds to a thread and has its own task pool. Internally the it has a global lock-free deque where tasks will all be submitted to. The `Scheduler` is responsible for managing the lifecycle of the worker threads and allocating tasks across all workers.

The worker loop (which runs concurrently on that worker's thread) is where tasks are executed. Tasks are obtained in this order:
- The worker's own deque.
- Inject from the global deque (in a batch).
- *Steal* task from another worker.

The whole point of a work-stealing architecture, where each worker has its own task pool and steal from each other, is scalability. When using only a global pool where all the workers will take tasks from, as the number of workers increases, that single deque becomes a synchronization and cache-contention hotspot. Using separate pools means that workers won't have to interfere with one another most of the time.

That is why the Chase-Lev deque is an integral part of the thread pool. It ensures each task pool is thread-safe without requiring a mutex around every operation. The deque is designed specifically for work stealing: its owning worker exclusively pushes and pops from one end, while other workers concurrently steal from the other end. This allows the common owner operations to proceed with minimal synchronization. Workers can then, usually, operate on their local task pools independently, preserving the scalability that motivated the work-stealing architecture in the first place.

A `ThreadPool` is a high level wrapper around a `Scheduler`, and is the primary interface that users interact with. It provides the `submit` method for submitting tasks to the scheduler, which returns a `Future` that can be used to await the task's completion.

# Benchmarks

Benchmarks were performed using Catch2 on a 20-thread machine (Release build, `-O3`). The work-stealing pool is evaluated against two baselines:
1. A standard worker pool with a single shared queue protected by `std::mutex` and `std::condition_variable`.
2. `std::async(std::launch::async, ...)`.

The benchmarks can be reproduced by running `make bench`. Full benchmark logs and raw Catch2 output are archived in [bench/RESULTS.md](bench/RESULTS.md).

### 1. High-throughput independent tasks (20 workers, 20,000 tasks)

Evaluates submission latency, task deque dispatch, and lock contention under high concurrency:

| Architecture | Execution Time (mean) | Throughput |
| :--- | :--- | :--- |
| **Work-Stealing Pool** | 21.50 ms | **~930,000 tasks/sec** |
| **Mutex Shared-Queue Pool** | 29.72 ms | ~672,000 tasks/sec |

* **Result:** The work-stealing pool achieves **~28% lower latency (1.38x higher throughput)**. As worker count increases, a centralized mutex queue becomes a severe synchronization bottleneck as all threads contend for the same lock.

### 2. Thread count scaling (CPU compute)

Evaluates speedup and scaling efficiency across 1, 2, 4, and 8 worker threads on 200 compute-bound tasks:

| Workers | Work-Stealing Pool | Mutex Pool | Work-Stealing Speedup |
| :---: | :---: | :---: | :---: |
| 1 | 152.07 ms | 156.05 ms | 1.00x (Baseline) |
| 2 | 76.66 ms | 77.66 ms | **1.98x** (Near-ideal linear) |
| 4 | 32.94 ms | 31.44 ms | **4.61x** (Superlinear cache scaling) |
| 8 | 10.75 ms | 11.82 ms | **14.14x** |

#### Analysis & Trade-offs
- **Work-Stealing wins when:** Under high thread counts (8+ threads) or fine-grained/imbalanced workloads, work stealing avoids centralized mutex contention by keeping tasks local to worker queues.
- **Mutex Pools win when:** On coarse, compute-heavy tasks with low thread counts (e.g., 4 workers), mutex contention is negligible. Here, the mutex pool can be slightly faster (31.4 ms vs 32.9 ms) because the worker loop only checks one queue, whereas work stealing incurs minor constant overhead checking local deque &rarr; global injection &rarr; victim worker deques.

### 3. Comparison with `std::async`

Evaluates asynchronous task execution overhead on batches of compute tasks:

| Framework | Execution Time (mean) | Speedup vs `std::async` |
| :--- | :--- | :--- |
| **Work-Stealing Pool** | 1.54 ms | **2.04x faster** |
| **`std::async(std::launch::async)`** | 3.14 ms | 1.00x |

This thread pool is **more than twice as fast** as `std::async`. Persistent worker threads and lock-free deques avoid the operating system thread creation/destruction costs associated with `std::launch::async`.
