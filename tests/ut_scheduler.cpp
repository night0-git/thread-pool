#include "utils.h"
#include "scheduler.h"
#include <chrono>

using scheduler::Scheduler;

TEST_CASE("Submit tasks to scheduler") {
    Scheduler s;

    INFO("Submit non void function");
    auto fut1 = s.submit([]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        return 10;
    });

    std::optional<int> res = std::nullopt;

    std::this_thread::sleep_for(std::chrono::milliseconds(450));
    res = fut1.poll();
    REQUIRE(!res.has_value());

    std::this_thread::sleep_for(std::chrono::milliseconds(550));
    res = fut1.poll();
    REQUIRE(res.has_value());
    REQUIRE(res.value() == 10);
}