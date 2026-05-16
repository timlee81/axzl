# Find threads for POSIX threading support
find_package(Threads REQUIRED)

# Create the axzl_core library from source files
add_library(axzl_core
    src/Mutex.cpp
)

target_include_directories(axzl_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    include/
)

# Link the threads library to the core library.
target_link_libraries(axzl_core PUBLIC
    Threads::Threads
)
