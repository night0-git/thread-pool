#pragma once

#include <thread_pool/detail/scheduler.h>
#include <thread_pool/detail/task.h>
#include <thread_pool/future.h>

namespace tp {

class ThreadPool {
private:
    detail::Scheduler scheduler;

public:
    ThreadPool(int num_workers) : scheduler(num_workers) {};

    template<class Function>
    // Function&& is a forwarding reference so that we
    // could deduce its type with std::invoke_result_t.
    auto submit(Function&& f);
};

// f could either be passed an lvalue, const lvalue or rvalue.
template <class Function>
inline auto ThreadPool::submit(Function&& f) {
    using Result = std::invoke_result_t<Function&>;

    auto state = std::make_shared<State<Result>>();
    Future<Result> fut(state);
    Promise<Result> prom(state);

    detail::Task* t = new detail::Task {
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

    scheduler.enqueue(std::unique_ptr<detail::Task>(t));

    return fut;
}

}