#include "scheduler.h"
#include <endian.h>
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
        // Check return condition.
        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [this] {
                return shutdown || pending_tasks > 0;
            });

            if (shutdown && pending_tasks == 0) {
                return;
            }
        }

        Task t;
        bool task_found = false;

        // Check local deque.
        {
            std::lock_guard<std::mutex> lock(
                workers[worker_id]->mtx
            );
            task_found = !workers[worker_id]->deque.empty();
            if (task_found) {
                t = std::move(workers[worker_id]->deque.front());
                workers[worker_id]->deque.pop_front();
            }
        }

        // No local task: find and steal task from another worker.
        if (!task_found) {
            for (size_t i = 0; i < workers.size(); i++) {
                if (i == worker_id) {
                    continue;
                }

                Worker& w = *workers[i];
                std::lock_guard<std::mutex> lock(w.mtx);
                if (!w.deque.empty()) {
                    t = std::move(w.deque.back());
                    w.deque.pop_back();
                    task_found = true;
                    break;
                }
            }
        }

        if (task_found) {
            // Task found inherently means a task was popped
            // from a deque so we decrement the counter here.
            {
                std::lock_guard<std::mutex> lock(mtx);
                pending_tasks--;
            }

            t.execute();
        }
    }
}