#include "scheduler.h"
using scheduler::Scheduler;

Scheduler::Scheduler(int num_workers) {
    workers.reserve(num_workers);
    for (int i = 0; i < num_workers; ++i) {
        workers.emplace_back(&Scheduler::worker_loop, this);
    }
}

Scheduler::~Scheduler() {
    {
        std::lock_guard<std::mutex> lock(mtx);
        shutdown = true;
    }
    cv.notify_all();

    for (auto& w : workers) {
        if (w.joinable()) {
            w.join();
        }
    }
}

void Scheduler::worker_loop() {
    while (true) {
        Task t;
        {
            std::unique_lock<std::mutex> lk(mtx);
            // Blocks the worker thread (this) until a task is
            // available or the destructor signals shutdown.
            cv.wait(lk, [this] {
                return shutdown || !queue.empty();
            });

            if (shutdown && queue.empty()) {
                return;
            }

            t = std::move(queue.front());
            queue.pop();
        }
        t.execute();
    }
}
