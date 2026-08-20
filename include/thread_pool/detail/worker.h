#pragma once

#include "deque/ring_buffer.h"
#include <thread_pool/detail/task.h>
#include <thread_pool/detail/deque.h>
#include <thread>

using task::Task;
using deque::TaskDeque;

namespace worker {

struct Worker {
    size_t id;
    TaskDeque deque;
    std::thread thread;

    explicit Worker(size_t id, size_t deque_capacity)
    : id(id), deque(deque_capacity) {};
};

}