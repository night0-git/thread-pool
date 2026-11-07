cmake_minimum_required(VERSION 3.23)

if (NOT TARGET Catch2::Catch2WithMain)
  include(FetchContent)
  FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.8.1
  )
  FetchContent_MakeAvailable(Catch2)
endif()

add_executable(benchmarks
  benchmarks.cpp
)

target_link_libraries(benchmarks PRIVATE
  thread_pool
  Catch2::Catch2WithMain
)

target_include_directories(benchmarks PRIVATE
  ${CMAKE_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}
)
