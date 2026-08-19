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
  ut_deque.cpp
  it_thread_pool.cpp
)
target_link_libraries(tests PRIVATE
  ${PROJECT_NAME}
  Catch2::Catch2WithMain
)
target_compile_definitions(tests PRIVATE TEST)