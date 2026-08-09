#include "deque/deque.h"
#include "task.h"

using deque::TaskDeque;
using task::Task;

bool TaskDeque::push(std::unique_ptr<Task> task) {
    const size_t b = bottom.load(utils::RELAXED);
    const size_t t = top.load(utils::ACQUIRE);

    auto* buf = buf_ref.load(utils::RELAXED);

    // Currently we make the deque bounded by capacity.
    // Pushing with no slot left returns false.
    if (b - t >= buf->capacity()) {
        return false;
    }

    buf->put(b, task.release());
    // Publish the push, and ensure that the pushed value
    // is visible to thieves.
    bottom.store(b + 1, utils::RELEASE);

    return true;
}

std::unique_ptr<Task> TaskDeque::pop() {
    size_t b = bottom.load(utils::RELAXED);
    if (b <= 0) {
        return nullptr;
    }

    auto* buf = buf_ref.load(utils::RELAXED);

    // Publish the pop beforehand to ensure correctness for
    // later operations
    bottom.store(b - 1, utils::RELAXED);

    std::atomic_thread_fence(utils::SEQ_CST);

    size_t t = top.load(utils::RELAXED);

    if (t >= b) {
        bottom.store(b, utils::RELAXED);
        return nullptr;
    } else if (t == b - 1) {
        // Race againsts thieves for the final item.
        if (!top.compare_exchange_strong(
            t, t + 1,
            utils::SEQ_CST,
            utils::RELAXED
        )) {
            // A thieve wins
            bottom.store(b, utils::RELAXED);
            return nullptr;
        }

        // Owner wins.
        bottom.store(b, utils::RELAXED);
    }

    return std::unique_ptr<Task>(buf->get(b - 1));
}

std::unique_ptr<Task> TaskDeque::steal() {
    size_t t = top.load(utils::ACQUIRE);

    auto buf = buf_ref.load(utils::RELAXED);

    std::atomic_thread_fence(utils::SEQ_CST);
    const size_t b = bottom.load(utils::ACQUIRE);

    if (t >= b) {
        return nullptr;
    }

    Task* stolen = buf->get(t);

    // Race againsts other thieves and the owner (a pop).
    if (!top.compare_exchange_strong(
        t, t + 1,
        utils::SEQ_CST,
        utils::RELAXED
    )) {
        // This thief loses.
        return nullptr;
    }

    return std::unique_ptr<Task>(stolen);
}