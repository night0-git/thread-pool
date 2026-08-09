#pragma once

#include "utils/mem_ord.h"
#include <atomic>

namespace task {
    struct Task;
}
using task::Task;

namespace deque {

class TaskRing {
private:
    size_t cap;

    std::unique_ptr<std::atomic<Task*>[]> slots {
        std::make_unique<std::atomic<Task*>[]>(cap)
    };
    std::unique_ptr<TaskRing> prev { nullptr };

public:
    explicit TaskRing(size_t capacity)
    : cap(capacity) {};

    size_t capacity() const { return cap; };

    void put(size_t index, Task* t) {
        slots[index % cap].store(t, utils::RELAXED);
    };

    Task* get(size_t index) const {
        return slots[index % cap].load(utils::RELAXED);
    };

    [[nodiscard]] TaskRing* grown(
        size_t top, size_t bottom
    ) const;
};

}