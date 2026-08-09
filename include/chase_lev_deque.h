#pragma once

#include <atomic>
#include <vector>
#include <optional>

namespace queue {

template<class T>
class Deque {
private:
    // Cache line padding to avoid false sharing, because
    // top and bottom are accessed frequently by different
    // threads.
    static constexpr size_t CACHE_LINE = 64;

    // Modified by the owner on push/pop.
    alignas(CACHE_LINE) std::atomic<ptrdiff_t> bottom { 0 };
    // Modified by thieves on steal primarily. The owner also
    // participates in updating top on the final item race.
    alignas(CACHE_LINE) std::atomic<ptrdiff_t> top { 0 };

    std::vector<T> buffer;
    ptrdiff_t cap;

public:
    explicit Deque(ptrdiff_t capacity);

    // Owner only: push to bottom.
    [[nodiscard]] bool push(T value);
    // Owner only: pop from bottom.
    [[nodiscard]] std::optional<T> pop();
    // Thieves only: pop from top.
    [[nodiscard]] std::optional<T> steal();

    [[nodiscard]] std::ptrdiff_t capacity() const;
};

template <class T>
inline Deque<T>::Deque(ptrdiff_t capacity) : cap(capacity) {
    buffer.resize(capacity);
}

template <class T>
inline bool Deque<T>::push(T value) {
    const ptrdiff_t b = bottom.load(std::memory_order_relaxed);
    const ptrdiff_t t = top.load(std::memory_order_acquire);

    // Currently we make the deque bounded by capacity.
    // Pushing with no slot left returns false.
    if (b - t >= cap) {
        return false;
    }

    buffer[b % cap] = std::move(value);
    // Publish the push, and ensure that the pushed value
    // is visible to thieves.
    bottom.store(b + 1, std::memory_order_release);

    return true;
}

template <class T>
inline std::optional<T> Deque<T>::pop() {
    ptrdiff_t b = bottom.load(std::memory_order_relaxed);
    // Publish the pop.
    bottom.store(b - 1, std::memory_order_relaxed);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    ptrdiff_t t = top.load(std::memory_order_relaxed);

    ptrdiff_t size = b - t;
    if (size <= 0) {
        bottom.store(b, std::memory_order_relaxed);
        return std::nullopt;
    } else if (size == 1) {
        // Race againsts thieves for the final item.
        if (!top.compare_exchange_strong(
            t, t + 1,
            std::memory_order_seq_cst,
            std::memory_order_relaxed
        )) {
            // A thieve wins
            bottom.store(b, std::memory_order_relaxed);
            return std::nullopt;
        }

        // Owner wins.
        bottom.store(b, std::memory_order_relaxed);
    }

    return std::move(buffer[(b - 1) % cap]);
}

template <class T>
inline std::optional<T> Deque<T>::steal() {
    ptrdiff_t t = top.load(std::memory_order_acquire);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    const ptrdiff_t b = bottom.load(std::memory_order_acquire);

    if (t >= b) {
        return std::nullopt;
    }

    T& stolen = buffer[t % cap];

    // Race againsts other thieves and the owner (a pop).
    if (!top.compare_exchange_strong(
        t, t + 1,
        std::memory_order_seq_cst,
        std::memory_order_relaxed
    )) {
        // This thief loses.
        return std::nullopt;
    }

    return std::move(stolen);
}

template <class T>
inline std::ptrdiff_t Deque<T>::capacity() const {
    return cap;
}

}