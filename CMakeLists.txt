cmake_minimum_required(VERSION 3.15)
project(load_gen CXX)

set (CMAKE_CXX_STANDARD 11)
add_subdirectory(external/toml11)

add_subdirectory(src)
