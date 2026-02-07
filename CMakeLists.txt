cmake_minimum_required(VERSION 3.20)

project(loadgen VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard to C++11 (matching current codebase)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set build type to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

option(BUILD_LOADGEN_GEN
    "Build the optional loadgen gen executable"
    OFF)

# Add toml11 library
add_subdirectory(external/toml11)

# Add source directory
add_subdirectory(src)
