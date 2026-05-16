cmake_minimum_required(VERSION 3.14)
project(axzl VERSION 1.0.0 LANGUAGES CXX)

# Force C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable testing framework
enable_testing()



# Add subdirectories
add_subdirectory(libaxzl_core)
add_subdirectory(test)
