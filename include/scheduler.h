#pragma once

#include "task.h"
#include "future.h"
#include "worker.h"
#include "deque/deque.h"
#include <mutex>
#include <condition_variable>
#include <vector>

using task::Task;
using future::Future, future::Promise;
using worker::Worker;
using deque::TaskDeque;

namespace scheduler {

constexpr size_t DEQUE_CAPACITY = 1024;

class Scheduler {
private:
    std::vector<std::unique_ptr<Worker>> workers;

    TaskDeque injector { DEQUE_CAPACITY };

    // Protects shutdown signal and pending task count.
    std::mutex mtx;
    // Blocks the worker until a task is available.
    std::condition_variable cv;
    bool shutdown { false };
    // Number of pending tasks across all workers and injector.
    size_t pending_tasks { 0 };

    void worker_loop(size_t worker_id);

public:
    explicit Scheduler(int num_workers);
    ~Scheduler();

    void enqueue(std::unique_ptr<Task> t);
};

}