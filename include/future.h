#pragma once

#include <optional>
#include <memory>
#include <mutex>
#include <condition_variable>

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
    std::condition_variable cv;
};

template<>
struct State<void> {
    Status status { Status::Pending };
    std::mutex mtx;
    std::condition_variable cv;
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
    T get();
    std::optional<T> try_get();
};

template<class T>
inline Status Future<T>::get_status() const {
    std::lock_guard lock(state->mtx);
    return state->status;
}

template<class T>
inline T Future<T>::get() {
    std::unique_lock lock(state->mtx);
    if (state->status == Status::Consumed) {
        throw std::runtime_error("Future already consumed");
    }

    state->cv.wait(lock, [this] {
        return state->status == Status::Ready &&
               state->value.has_value();
    });

    auto val = std::move(state->value);
    state->value = std::nullopt;
    state->status = Status::Consumed;
    return val.value();
}

template<class T>
inline std::optional<T> Future<T>::try_get() {
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
    void get();
    bool try_get();
};

inline Status Future<void>::get_status() const {
    std::lock_guard lock(state->mtx);
    return state->status;
}

inline void Future<void>::get() {
    std::unique_lock lock(state->mtx);
    if (state->status == Status::Consumed) {
        throw std::runtime_error("Future already consumed");
    }

    state->cv.wait(lock, [this] {
        return state->status == Status::Ready;
    });
    state->status = Status::Consumed;
}

inline bool Future<void>::try_get() {
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
        state->cv.notify_one();
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
        state->cv.notify_one();
        return true;
    }
    return false;
}

}