#pragma once

#include "future.h"
#include <functional>

using future::Future;

namespace task {

using TaskAction = std::function<void()>;

enum class Priority {
    Low,
    Medium,
    High,
};

struct Task {
    TaskAction execute;
    Priority prio { Priority::Medium };
};

}