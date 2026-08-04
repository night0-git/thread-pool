#pragma once

#include <optional>
#include <memory>

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
};

template<class T>
class Future {
private:
    std::shared_ptr<State<T>> state;

public:
    Future() = delete;
    Future(std::shared_ptr<State<T>> state) : state(state) {}

    Status get_status() const;
    std::optional<T> poll();
    bool complete(T value);
};

template<class T>
inline Status Future<T>::get_status() const {
    return state->status;
}

// Non-blocking poll.
template<class T>
inline std::optional<T> Future<T>::poll() {
    if (state->status == Status::Ready) {
        state->status = Status::Consumed;
        auto val = std::move(state->value);
        state->value = std::nullopt;
        return val;
    }
    return std::nullopt;
}

template<class T>
inline bool Future<T>::complete(T value) {
    if (state->status != Status::Pending) {
        return false;
    }
    state->value = value;
    state->status = Status::Ready;
    return true;
}

}