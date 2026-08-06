#pragma once

#include <optional>
#include <memory>
#include <mutex>

namespace future {

enum class Status {
    Pending,
    Ready,
    Consumed,
};

template<class T>
struct State {
    Status status { Status::Pending };
    std::optional<T> value { std::nullopt };
    std::mutex mtx;
};

template<>
struct State<void> {
    Status status { Status::Pending };
    std::mutex mtx;
};

template<class T>
class Future {
private:
    std::shared_ptr<State<T>> state;

public:
    Future() = delete;
    explicit Future(std::shared_ptr<State<T>> state)
        : state(std::move(state)) {}

    Status get_status() const;
    std::optional<T> poll();
};

template<class T>
inline Status Future<T>::get_status() const {
    std::lock_guard lock(state->mtx);
    return state->status;
}

// Non-blocking poll.
template<class T>
inline std::optional<T> Future<T>::poll() {
    std::lock_guard lock(state->mtx);
    if (state->status == Status::Ready) {
        state->status = Status::Consumed;
        auto val = std::move(state->value);
        state->value = std::nullopt;
        return val;
    }
    return std::nullopt;
}

template<>
class Future<void> {
private:
    std::shared_ptr<State<void>> state;

public:
    Future() = delete;
    explicit Future(std::shared_ptr<State<void>> state)
        : state(std::move(state)) {}

    Status get_status() const;
    bool poll();
};

inline Status Future<void>::get_status() const {
    std::lock_guard lock(state->mtx);
    return state->status;
}

// Non-blocking poll.
inline bool Future<void>::poll() {
    std::lock_guard lock(state->mtx);
    if (state->status == Status::Ready) {
        state->status = Status::Consumed;
        return true;
    }
    return false;
}

template<class T>
class Promise {
private:
    std::shared_ptr<State<T>> state;

public:
    Promise() = delete;
    explicit Promise(std::shared_ptr<State<T>> state)
        : state(std::move(state)) {}

    bool complete(T value);
};

template<class T>
inline bool Promise<T>::complete(T value) {
    std::lock_guard lk(state->mtx);
    if (state->status == Status::Pending) {
        state->status = Status::Ready;
        state->value = std::move(value);
        return true;
    }
    return false;
}

template<>
class Promise<void> {
private:
    std::shared_ptr<State<void>> state;

public:
    Promise() = delete;
    explicit Promise(std::shared_ptr<State<void>> state)
        : state(std::move(state)) {}

    bool complete();
};

inline bool Promise<void>::complete() {
    std::lock_guard lk(state->mtx);
    if (state->status == Status::Pending) {
        state->status = Status::Ready;
        return true;
    }
    return false;
}

}