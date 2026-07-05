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
set_property(CACHE AXZL_UNRECOVERABLE_ERROR_POLICY PROPERTY STRINGS "throw" "log" "ignore" "terminate")

# Define preprocessor definition based on selected option
if(AXZL_UNRECOVERABLE_ERROR_POLICY STREQUAL "throw")
    add_compile_definitions(AXZL_UNRECOVERABLE_ERROR_POLICY_THROW)
elseif(AXZL_UNRECOVERABLE_ERROR_POLICY STREQUAL "log")
    add_compile_definitions(AXZL_UNRECOVERABLE_ERROR_POLICY_LOG)
elseif(AXZL_UNRECOVERABLE_ERROR_POLICY STREQUAL "ignore")
    add_compile_definitions(AXZL_UNRECOVERABLE_ERROR_POLICY_IGNORE)
elseif(AXZL_UNRECOVERABLE_ERROR_POLICY STREQUAL "terminate")
    add_compile_definitions(AXZL_UNRECOVERABLE_ERROR_POLICY_TERMINATE)
else()
    add_compile_definitions(AXZL_UNRECOVERABLE_ERROR_POLICY_THROW)
endif()

# Enable testing framework
enable_testing()



# Add subdirectories
add_subdirectory(libaxzl_core)
add_subdirectory(test)
