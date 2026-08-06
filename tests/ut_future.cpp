#include "utils.h"
#include "future.h"
using future::Future, future::Status, future::Promise;

TEST_CASE("Non-void future complete and poll") {
    auto state = std::make_shared<future::State<int>>();
    Future<int> fut(state);
    Promise<int> prom(state);

    REQUIRE(!fut.poll().has_value());
    REQUIRE(fut.get_status() == Status::Pending);

    REQUIRE(prom.complete(10));
    REQUIRE(fut.get_status() == Status::Ready);
    REQUIRE(fut.poll() == 10);
    REQUIRE(fut.get_status() == Status::Consumed);

    REQUIRE(!prom.complete(15));
    REQUIRE(!fut.poll().has_value());
}

TEST_CASE("void future complete and poll") {
    auto state = std::make_shared<future::State<void>>();
    Future<void> fut(state);
    Promise<void> prom(state);

    REQUIRE(!fut.poll());
    REQUIRE(fut.get_status() == Status::Pending);

    REQUIRE(prom.complete());
    REQUIRE(fut.get_status() == Status::Ready);
    REQUIRE(fut.poll());
    REQUIRE(fut.get_status() == Status::Consumed);

    REQUIRE(!prom.complete());
    REQUIRE(!fut.poll());
}