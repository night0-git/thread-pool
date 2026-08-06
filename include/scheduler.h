#pragma once

#include "task.h"
#include "future.h"
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>

using task::Task;
using future::Future, future::Promise;

namespace scheduler {

class Scheduler {
private:
    std::queue<Task> queue {};
    std::vector<std::thread> workers;
    // Protects queue and shutdown.
    std::mutex mtx;
    // Blocks the worker until a task is available.
    std::condition_variable cv;
    bool shutdown { false };

    void worker_loop();

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

    Task t = Task {
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

    {
        std::lock_guard lock(mtx);
        queue.push(std::move(t));
    }
    // Notify the worker thread that a task is available.
    cv.notify_one();
    return fut;
}

}