#pragma once

#include "task.h"
#include "future.h"
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

using task::Task;
using future::Future;

namespace scheduler {

class Scheduler {
private:
    std::queue<Task> queue {};
    std::jthread worker;
    std::mutex mtx;    // Protects queue and shutdown.
    std::condition_variable cv;
    bool shutdown { false };

    void worker_loop();

public:
    // We pass as a reference combined with 'this' because
    // worker_loop is a non-static member function and needs
    // and object to operate on.
    Scheduler() : worker(&Scheduler::worker_loop, this) {};
    ~Scheduler();

    template<class Function>
    // Function&& is a forward reference that so we
    // could deduce its type with std::invoke_result_t.
    auto submit(Function&& f);
};

// f is a forwarding reference (because Function is generic)
// and it could either be passed an lvalue, const lvalue or
// rvalue.
template <class Function>
inline auto Scheduler::submit(Function&& f) {
    using Result = std::invoke_result_t<Function>;

    auto state = std::make_shared<future::State<Result>>();
    Future<Result> fut(state);

    Task t = Task {
        .execute = [
            // We use std::forward to preserve value category
            // for f.
            f = std::forward<Function>(f),
            state
        ]() {
            Result res = std::invoke(f);
            state->value = std::move(res);
            state->status = future::Status::Ready;
        },
    };

    {
        std::lock_guard<std::mutex> lock(mtx);
        queue.push(t);
    }
    // Notify the worker thread that a task is available.
    cv.notify_one();
    return fut;
}

}