#pragma once

#include "utils/mem_ord.h"
#include <atomic>

namespace tp::detail {

struct Task;

// Instance counter used by tests to verify that old buffers
// (from resizes) are reclaimed. Always compiled so that the
// class layout stays identical across TUs, the cost is two
// relaxed atomic ops per ring creation/destruction.
inline std::atomic<int> num_instances { 0 };

class TaskRing {
private:
    size_t cap;

    std::unique_ptr<std::atomic<Task*>[]> slots {
        std::make_unique<std::atomic<Task*>[]>(cap)
    };
    std::unique_ptr<TaskRing> prev { nullptr };

public:
    explicit TaskRing(size_t capacity)
    : cap(capacity) {
        num_instances.fetch_add(1, std::memory_order_relaxed);
    };

    ~TaskRing() {
        num_instances.fetch_sub(1, std::memory_order_relaxed);
    };

    size_t capacity() const { return cap; };

    void put(size_t index, Task* t) {
        slots[index % cap].store(t, utils::RELAXED);
    };

    Task* get(size_t index) const {
        return slots[index % cap].load(utils::RELAXED);
    };

    [[nodiscard]] static std::unique_ptr<TaskRing> grow(
        std::unique_ptr<TaskRing> old_ring,
        size_t top, size_t bottom
    );
};

}