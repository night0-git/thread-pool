#pragma once

#include <functional>

namespace tp::detail {

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