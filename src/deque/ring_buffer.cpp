#include "deque/ring_buffer.h"

using deque::TaskRing;

std::unique_ptr<TaskRing> TaskRing::grow(
    std::unique_ptr<TaskRing> old_ring,
    size_t top, size_t bottom
) {
    auto* new_ring = new TaskRing(2 * old_ring->capacity());

    for (size_t i = top; i < bottom; i++) {
        new_ring->put(i, old_ring->get(i));
    }

    new_ring->prev = std::move(old_ring);

    return std::unique_ptr<TaskRing>(new_ring);
}