#include "utils.h"
#include "scheduler.h"

using scheduler::Scheduler;

TEST_CASE("Submit tasks to scheduler and run") {
    Scheduler s;

    INFO("Submit non void function");
    auto fut1 = s.submit([]() {
        return 10;
    });
    REQUIRE(fut1.get_status() == future::Status::Pending);
    REQUIRE(!fut1.poll().has_value());
    s.run_one();

    auto res = fut1.poll();
    REQUIRE(res.has_value());
    REQUIRE(res.value() == 10);
}