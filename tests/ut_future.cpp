#include "utils.h"
#include <thread_pool/future.h>

using tp::Future, tp::Status, tp::Promise;

TEST_CASE("Non-void future complete and poll", "[future]") {
    auto state = std::make_shared<tp::State<int>>();
    Future<int> fut(state);
    Promise<int> prom(state);

    REQUIRE_FALSE(fut.try_get().has_value());
    REQUIRE(fut.get_status() == Status::Pending);

    REQUIRE(prom.complete(10));
    REQUIRE(fut.get_status() == Status::Ready);
    REQUIRE(fut.get() == 10);
    REQUIRE(fut.get_status() == Status::Consumed);

    REQUIRE_FALSE(prom.complete(15));
    REQUIRE_THROWS(fut.get());
}

TEST_CASE("void future complete and poll", "[future]") {
    auto state = std::make_shared<tp::State<void>>();
    Future<void> fut(state);
    Promise<void> prom(state);

    REQUIRE_FALSE(fut.try_get());
    REQUIRE(fut.get_status() == Status::Pending);

    REQUIRE(prom.complete());
    REQUIRE(fut.get_status() == Status::Ready);
    REQUIRE_NOTHROW(fut.get());
    REQUIRE(fut.get_status() == Status::Consumed);

    REQUIRE_FALSE(prom.complete());
    REQUIRE_THROWS(fut.get());
}

TEST_CASE("Future get blocking", "[future]") {
    auto state = std::make_shared<tp::State<int>>();
    Future<int> fut(state);
    Promise<int> prom(state);

    REQUIRE_FALSE(fut.try_get().has_value());
    REQUIRE(fut.get_status() == Status::Pending);

    std::thread([p = std::move(prom)] mutable {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        REQUIRE(p.complete(10));
    }).detach();

    REQUIRE(fut.get() == 10);
    REQUIRE(fut.get_status() == Status::Consumed);
}