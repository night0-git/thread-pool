#pragma once

#include "task.h"
#include <thread>
#include <deque>
#include <mutex>
using task::Task;

namespace worker {

struct Worker {
    size_t id;
    std::deque<Task> deque;
    std::mutex mtx;
    std::thread thread;

    explicit Worker(size_t id) : id(id) {};
};

}