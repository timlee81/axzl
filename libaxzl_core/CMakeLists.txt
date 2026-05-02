cmake_minimum_required(VERSION 3.10)
project(libaxzl_core CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find threads for POSIX threading support
find_package(Threads REQUIRED)

# Header-only library (Mutex is currently header-only with empty implementations)
add_library(axzl_core INTERFACE)
target_include_directories(axzl_core INTERFACE include/)
target_link_libraries(axzl_core INTERFACE Threads::Threads)

# Test executable (assumes test/Mutex.cpp is a test file; may need Catch2 or similar for proper unit tests)
add_executable(test_mutex test/Mutex.cpp)
target_link_libraries(test_mutex axzl_core)

# Optional: Add a test target
enable_testing()
add_test(NAME MutexTest COMMAND test_mutex)