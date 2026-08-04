cmake_minimum_required(VERSION 3.23)

Include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.8.1
)

FetchContent_MakeAvailable(Catch2)

add_executable(tests
  ut_future.cpp
  ut_scheduler.cpp
)
target_link_libraries(tests PRIVATE core_lib Catch2::Catch2WithMain)