#include "scheduler.h"
using scheduler::Scheduler;

void Scheduler::run_one() {
    if (queue.empty()) {
        return;
    }
    queue.front().execute();
    queue.pop();
}

void Scheduler::run_all() {
    while (!queue.empty()) {
        run_one();
    }
}