add_library(request
    random.cpp
    request_generation.cpp
)

target_include_directories(request
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link zlib statically
target_link_libraries(request
    PUBLIC
        types
        toml11
)