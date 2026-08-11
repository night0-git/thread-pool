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

    void enqueue(std::unique_ptr<Task> t);
    void worker_loop(size_t worker_id);

public:
    // We pass as a reference combined with 'this' because
    // worker_loop is a non-static member function and needs
    // and object to operate on.
    explicit Scheduler(int num_workers);
    ~Scheduler();

    template<class Function>
    // Function&& is a forwarding reference so that we
    // could deduce its type with std::invoke_result_t.
    auto submit(Function&& f);
};

// f could either be passed an lvalue, const lvalue or rvalue.
template <class Function>
inline auto Scheduler::submit(Function&& f) {
    using Result = std::invoke_result_t<Function&>;

    auto state = std::make_shared<future::State<Result>>();
    Future<Result> fut(state);
    Promise<Result> prom(state);

    Task* t = new Task {
        .execute = [
            // We use std::forward to preserve f's value category
            f = std::forward<Function>(f),
            p = std::move(prom)
        ]() mutable {
            if constexpr (std::is_void_v<Result>) {
                std::invoke(f);
                p.complete();
            } else {
                Result res = std::invoke(f);
                p.complete(std::move(res));
            }
        },
    };

    enqueue(std::unique_ptr<Task>(t));

    return fut;
}

}