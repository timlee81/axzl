cmake_minimum_required(VERSION 3.14)
project(axzl VERSION 1.0.0 LANGUAGES CXX)

# Find threads for POSIX threading support
find_package(Threads REQUIRED)

# Find fmt for formatting
find_package(fmt REQUIRED)


# Force C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# AXZL_UNRECOVERABLE_ERROR_POLICY
set(AXZL_UNRECOVERABLE_ERROR_POLICY "throw" CACHE STRING "Select Unrecoverable Error Policy option")
set_property(CACHE AXZL_UNRECOVERABLE_ERROR_POLICY PROPERTY STRINGS "throw" "logAndThrow" "logAndCallHandler" "logAndAbort")

# Define preprocessor definition based on selected option
if(AXZL_UNRECOVERABLE_ERROR_POLICY STREQUAL "throw")
    add_compile_definitions(AXZL_UNRECOVERABLE_ERROR_POLICY_THROW)
elseif(AXZL_UNRECOVERABLE_ERROR_POLICY STREQUAL "logAndThrow")
    add_compile_definitions(AXZL_UNRECOVERABLE_ERROR_POLICY_LOG_AND_THROW)
elseif(AXZL_UNRECOVERABLE_ERROR_POLICY STREQUAL "logAndCallHandler")
    add_compile_definitions(AXZL_UNRECOVERABLE_ERROR_POLICY_LOG_AND_CALL_HANDLER)
elseif(AXZL_UNRECOVERABLE_ERROR_POLICY STREQUAL "logAndAbort")
    add_compile_definitions(AXZL_UNRECOVERABLE_ERROR_POLICY_LOG_AND_ABORT)
else()
    add_compile_definitions(AXZL_UNRECOVERABLE_ERROR_POLICY_THROW)
endif()

# Enable testing framework
enable_testing()



# Add subdirectories
add_subdirectory(libaxzl_core)
add_subdirectory(test)
