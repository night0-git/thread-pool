cmake_minimum_required(VERSION 3.23)
project(thread-pool
  VERSION 0.1.0
  LANGUAGES CXX
)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE
  ${CMAKE_SOURCE_DIR}/bin/release
)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG
  ${CMAKE_SOURCE_DIR}/bin/debug
)

# Project-wide flags
add_library(project_options INTERFACE)
target_compile_options(project_options INTERFACE
  -Wall -Wextra -Wpedantic
)
if (ENABLE_TSAN)
  target_compile_options(project_options INTERFACE -fsanitize=thread)
  target_link_options(project_options INTERFACE -fsanitize=thread)
endif()

# The core library
add_library(thread_pool
  src/deque/deque.cpp
  src/deque/ring_buffer.cpp
  src/scheduler.cpp
)
add_library(thread_pool::thread_pool ALIAS thread_pool)
target_include_directories(thread_pool
  PUBLIC
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  PRIVATE
    # Internal implementation headers are put in src/.
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_compile_features(thread_pool PUBLIC cxx_std_23)
target_link_libraries(thread_pool PRIVATE
  $<BUILD_INTERFACE:project_options>
)
# Specify public headers into a FILE_SET
target_sources(thread_pool
  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS include
    FILES
      include/thread_pool/thread_pool.h
      include/thread_pool/future.h
      include/thread_pool/detail/deque.h
      include/thread_pool/detail/scheduler.h
      include/thread_pool/detail/task.h
      include/thread_pool/detail/worker.h
)

install(
  TARGETS thread_pool
  EXPORT thread_poolTargets

  FILE_SET HEADERS DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(
  EXPORT thread_poolTargets
  FILE thread_poolTargets.cmake
  NAMESPACE thread_pool::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/thread_pool
)

configure_package_config_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/thread_poolConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/thread_poolConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/thread_pool
)

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/thread_poolConfigVersion.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

install(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/thread_poolConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/thread_poolConfigVersion.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/thread_pool
)

add_subdirectory(tests)