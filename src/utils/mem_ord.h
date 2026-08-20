#pragma once

#include <memory> // IWYU pragma: export

namespace utils {
    constexpr std::memory_order RELAXED = std::memory_order_relaxed;
    constexpr std::memory_order ACQUIRE = std::memory_order_acquire;
    constexpr std::memory_order RELEASE = std::memory_order_release;
    constexpr std::memory_order SEQ_CST = std::memory_order_seq_cst;
}