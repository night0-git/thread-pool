#include "utils.h"
#include "future.h"
using future::Future, future::Status;

TEST_CASE("Future complete and poll") {
    Future<int> fut(std::make_shared<future::State<int>>());
    REQUIRE(!fut.poll().has_value());
    REQUIRE(fut.get_status() == Status::Pending);

    int val = 10;
    REQUIRE(fut.complete(val));
    REQUIRE(fut.get_status() == Status::Ready);
    REQUIRE(fut.poll() == val);
    REQUIRE(fut.get_status() == Status::Consumed);

    REQUIRE(!fut.complete(val + 5));
    REQUIRE(!fut.poll().has_value());
}