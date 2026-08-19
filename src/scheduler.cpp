#include "scheduler.h"
#include <endian.h>
using scheduler::Scheduler;

Scheduler::Scheduler(int num_workers) {
    workers.reserve(num_workers);
    constexpr size_t worker_deque_cap = 1024;
    for (int i = 0; i < num_workers; ++i) {
        workers.emplace_back(std::make_unique<Worker>(
            i, worker_deque_cap
        ));
        // We pass as a reference combined with 'this' because
        // worker_loop is a non-static member function and needs
        // and object to operate on.
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

void Scheduler::enqueue(std::unique_ptr<Task> t) {
    injector.push(std::move(t));

    {
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

        std::unique_ptr<Task> t;

        // Source 1: Check local deque.
        t = workers[worker_id]->deque.pop();

        // Source 2: Inject tasks from global pool in a batch.
        if (!t && (t = injector.steal())) {
            // Prepare this worker with more tasks.
            size_t batch_size = 8;
            for (size_t i = 0; i < batch_size; i++) {
                if (auto stolen = injector.steal()) {
                    workers[worker_id]->deque.push(
                        std::move(stolen)
                    );
                } else {
                    break;
                }
            }
        }

        // Source 3: Steal task from another worker.
        if (!t) {
            for (size_t i = 0; i < workers.size(); i++) {
                if (i == worker_id) {
                    continue;
                }

                if ((t = workers[i]->deque.steal())) {
                    break;
                }
            }
        }

        if (t) {
            // Task found inherently means a task was popped
            // from a deque so we decrement the counter here.
            {
                std::lock_guard<std::mutex> lock(mtx);
                pending_tasks--;
            }

            t->execute();
        }
    }
}