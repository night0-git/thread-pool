#include "deque/ring_buffer.h"

using deque::TaskRing;

TaskRing* TaskRing::grown(size_t top, size_t bottom) const {
    auto* new_ring = new TaskRing(2 * cap);

    for (size_t i = top; i < bottom; i++) {
        new_ring->put(i, get(i));
    }

    return new_ring;
}