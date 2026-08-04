#pragma once

#include "task.h"
#include "future.h"
#include <queue>

using task::Task;
using future::Future;

namespace scheduler {

class Scheduler {
private:
    std::queue<Task> queue {};
public:
    Scheduler() = default;

    template<class Function>
    auto submit(Function&& f);
    void run_one();
    void run_all();
};

// f is a forward reference and it could either be
// passed an lvalue or an rvalue.
template <class Function>
auto Scheduler::submit(Function&& f) {
    using Result = std::invoke_result_t<Function>;

    auto state = std::make_shared<future::State<Result>>();
    Future<Result> fut(state);

    Task t = Task {
        .execute = [
            // Therefore we use std::forward to preserve value category
            f = std::forward<Function>(f),
            state
        ]() {
            Result res = std::invoke(f);
            state->value = std::move(res);
            state->status = future::Status::Ready;
        },
    };

    queue.push(t);
    return fut;
}

}