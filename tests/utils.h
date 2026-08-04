#pragma once

#include <catch2/catch_test_macros.hpp>

#define REQUIRE_MSG(cond, msg) do { INFO(msg); REQUIRE(cond); } while(false)
#define CHECK_MSG(cond, msg)   do { INFO(msg); CHECK(cond); } while(false)