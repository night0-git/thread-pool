#pragma once

#include "deque/ring_buffer.h"
#include <atomic>

namespace tp::detail {

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

    // The newest buffer.
    // Used to manage buffer lifetime, once this deque is
    // destroyed, all the old buffers (from resizes) are
    // destroyed through the chain starting from this.
    // Because only the owner accesses latest_buf, it does
    // not need to be atomic.
    std::unique_ptr<TaskRing> latest_buf;
    // The atomically published buffer. Because the owner can
    // replace the buffer during resizing this may not point
    // to the latest buffer (latest_buf).
    std::atomic<TaskRing*> buf_ref { latest_buf.get() };

public:
    explicit TaskDeque(ptrdiff_t capacity)
    : latest_buf(std::make_unique<TaskRing>(capacity)) {}

    // Owner only: push to bottom.
    void push(std::unique_ptr<Task> task);
    // Owner only: pop from bottom.
    [[nodiscard]] std::unique_ptr<Task> pop();
    // Thieves only: pop from top.
    [[nodiscard]] std::unique_ptr<Task> steal();

    size_t capacity() const { return latest_buf->capacity(); }
};

}