cmake_minimum_required(VERSION 3.10)
project(axzl CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add subprojects
add_subdirectory(libaxzl_core)
add_subdirectory(test)