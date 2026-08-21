#pragma once

#include "utils/mem_ord.h"
#include <atomic>

namespace tp::detail {

struct Task;

// Test-only global variable to track the number of TaskRing
// instances, which corresponds to the number of deque resizes.
#ifdef TEST
inline std::atomic<int> num_instances { 0 };
#endif

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
        #ifdef TEST
        num_instances.fetch_add(1, std::memory_order_relaxed);
        #endif
    };

    #ifdef TEST
    ~TaskRing() {
        num_instances.fetch_sub(1, std::memory_order_relaxed);
    };
    #endif

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