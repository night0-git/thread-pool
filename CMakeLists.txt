cmake_minimum_required(VERSION 3.23)
project(task-scheduler LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE
  ${CMAKE_SOURCE_DIR}/bin/release
)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG
  ${CMAKE_SOURCE_DIR}/bin/debug
)

add_library(project_options INTERFACE)
target_compile_options(project_options INTERFACE
  -Wall -Wextra -Wpedantic
)

add_library(${PROJECT_NAME}
  src/scheduler.cpp
)
target_include_directories(${PROJECT_NAME} PUBLIC include)
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_23)
target_link_libraries(${PROJECT_NAME} PRIVATE project_options)

add_subdirectory(tests)