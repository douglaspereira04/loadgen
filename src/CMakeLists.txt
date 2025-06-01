add_subdirectory(request)
add_subdirectory(types)

add_executable(gen gen.cpp)

target_include_directories(gen
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(
    gen
        PRIVATE
            toml11
            request
            types
)