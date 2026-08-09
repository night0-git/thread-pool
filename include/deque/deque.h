#pragma once

#include "ring_buffer.h"
#include <atomic>

namespace deque {

class TaskDeque {
private:
    // Cache line padding to avoid false sharing, because
    // top and bottom are accessed frequently by different
    // threads.
    static constexpr size_t CACHE_LINE = 64;

    // Modified by the owner on push/pop.
    alignas(CACHE_LINE) std::atomic<size_t> bottom { 0 };
    // Modified by thieves on steal primarily. The owner also
    // participates in updating top on the final item race.
    alignas(CACHE_LINE) std::atomic<size_t> top { 0 };

    // Used to manage buffer lifetime, once this deque is
    // destroyed, all the old buffers (from resizes) are
    // destroyed through the chain.
    std::unique_ptr<TaskRing> buf_obj;
    // Reference to the newest buffer, used by thieves to
    // perform atomic operations.
    std::atomic<TaskRing*> buf_ref;

public:
    explicit TaskDeque(ptrdiff_t capacity)
    : buf_obj(std::make_unique<TaskRing>(capacity)) {}

    // Owner only: push to bottom.
    [[nodiscard]] bool push(std::unique_ptr<Task> task);
    // Owner only: pop from bottom.
    [[nodiscard]] std::unique_ptr<Task> pop();
    // Thieves only: pop from top.
    [[nodiscard]] std::unique_ptr<Task> steal();
};

}