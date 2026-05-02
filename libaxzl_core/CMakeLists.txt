cmake_minimum_required(VERSION 3.10)
project(libaxzl_core CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find threads for POSIX threading support
find_package(Threads REQUIRED)

# Create the axzl_core library from source files
add_library(axzl_core src/Mutex.cpp)
target_include_directories(axzl_core PUBLIC include/)
target_link_libraries(axzl_core PUBLIC Threads::Threads)
