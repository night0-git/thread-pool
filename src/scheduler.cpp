#include "scheduler.h"
using scheduler::Scheduler;

Scheduler::~Scheduler() {
    {
        std::lock_guard<std::mutex> lock(mtx);
        shutdown = true;
    }

    cv.notify_one();
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
            lk.unlock();
        }
        t.execute();
    }
}
