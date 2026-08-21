#include <thread_pool/detail/deque.h>
#include <thread_pool/detail/task.h>

namespace tp::detail {

void TaskDeque::push(std::unique_ptr<Task> task) {
    const size_t b = bottom.load(utils::RELAXED);
    const size_t t = top.load(utils::ACQUIRE);

    auto* buf = buf_ref.load(utils::RELAXED);

    // Grow the buffer.
    if (b - t >= buf->capacity()) {
        latest_buf = TaskRing::grow(std::move(latest_buf), t, b);
        // Any thieves currently using buf_ref will see
        // the old address, which is valid because we do
        // not destroy it.
        buf_ref.store(latest_buf.get(), utils::RELEASE);
        buf = latest_buf.get();
    }

    buf->put(b, task.release());
    // Publish the push, and ensure that the pushed value
    // is visible to thieves.
    bottom.store(b + 1, utils::RELEASE);
}

std::unique_ptr<Task> TaskDeque::pop() {
    size_t b = bottom.load(utils::RELAXED);
    if (b <= 0) {
        return nullptr;
    }

    auto* buf = buf_ref.load(utils::ACQUIRE);

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

    auto buf = buf_ref.load(utils::ACQUIRE);

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

}