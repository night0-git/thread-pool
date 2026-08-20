# Introduction

This is an educational work-stealing thread pool in C++ for me to learn more about concurrency primitives such as future, promise, threads, and also lock-free programming.

# Project overview

The interface provided is simply a `ThreadPool` for user to submit tasks to be executed on the background, and a `Future` type to poll the result of the task.

The core mechanic of task allocation is that a thread will try to steal from other threads after it has exhausted its own pool. A lock-free single producer - multiple consumer (SPMC) deque is used to hold tasks, which was the hardest part to implement.

# Usage
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
>This part is for those who may be insterested in how the code works (or for myself in the future).

### 1. Custom future

A simplified version of the standard library's `future` and `promise` is implemented for this project: `Future` and `Promise`.

How they work (together): A `Future` and a `Promise` share a unified `State` which both can read and write to. `Future` is held by the customer and it contains method to poll the result, while a `Promise` is what the producer uses to "complete" its corresponding `Future`. In the code, a conditional variable to used by the `Future` to block the thread while waiting for `Promise` to complete it (during poll).

A future is the result of a submitted task, so it may not contain a value (void). In C++ this is handled through 'template specialization' for `void` type.

### 2. Lock-free SPMC deque

Also called a Chase-Lev deque, it is a double-ended queue implemented purely with atomic primitives, which makes it completely thread-safe without a mutex.

A deque provides a `pop`/`push` method that only the owner thread can use to take tasks from the bottom, and a `steal` method that any thread (thief) can use to steal tasks from the top. This separation means that the owner and thieves normally operate on different ends of the deque. The only case where they may contend for the same task is when exactly one task remains, which can be resolved by a CAS.

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