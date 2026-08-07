#include "scheduler.h"
using scheduler::Scheduler;

Scheduler::Scheduler(int num_workers) {
    workers.reserve(num_workers);
    for (int i = 0; i < num_workers; ++i) {
        workers.emplace_back(std::make_unique<Worker>(i));
        workers[i]->thread = std::thread(
            &Scheduler::worker_loop, this, i
        );
    }
}

Scheduler::~Scheduler() {
    {
        std::lock_guard<std::mutex> lock(mtx);
        shutdown = true;
    }
    cv.notify_all();

    for (auto& w : workers) {
        if (w->thread.joinable()) {
            w->thread.join();
        }
    }
}

void Scheduler::enqueue(Task t) {
    // Assign the task to a worker in a round robin manner.
    size_t id = last_worker_id.fetch_add(1) % workers.size();
    {
        // Acquire the worker's internal mutex to access its deque.
        std::lock_guard lock(workers[id]->mtx);
        workers[id]->deque.push_front(std::move(t));
    }

    {
        // Acquire the scheduler's mutex to update pending tasks.
        std::lock_guard lock(mtx);
        pending_tasks++;
    }
    cv.notify_one();
}

void Scheduler::worker_loop(size_t worker_id) {
    while (true) {
        Task t;
        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [this] {
                return shutdown || pending_tasks > 0;
            });

            if (shutdown && pending_tasks == 0) {
                return;
            }
        }

        {
            std::lock_guard<std::mutex> lock(workers[worker_id]->mtx);

            // Currently there is no work stealing yet, so
            // we simply skip this iteration.
            if (workers[worker_id]->deque.empty()) {
                continue;
            }

            t = std::move(workers[worker_id]->deque.front());
            workers[worker_id]->deque.pop_front();
        }

        t.execute();

        {
            std::lock_guard<std::mutex> lock(mtx);
            pending_tasks--;
        }
    }
}
