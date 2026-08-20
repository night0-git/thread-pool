#pragma once

#include <thread_pool/detail/task.h>
#include <thread_pool/detail/worker.h>
#include <thread_pool/detail/deque.h>
#include <vector>

using task::Task;
using worker::Worker;
using deque::TaskDeque;

namespace scheduler {

constexpr size_t DEQUE_CAPACITY = 1024;

class Scheduler {
private:
    std::vector<std::unique_ptr<Worker>> workers;

    TaskDeque injector { DEQUE_CAPACITY };

    std::atomic<uint32_t> state { 0 };
    static constexpr uint32_t SHUTDOWN_BIT = 1u << 31;
    static constexpr uint32_t COUNT_MASK = ~SHUTDOWN_BIT;

    void worker_loop(size_t worker_id);

public:
    explicit Scheduler(int num_workers);
    ~Scheduler();

    void enqueue(std::unique_ptr<Task> t);
};

}