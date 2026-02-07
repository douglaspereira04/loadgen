add_library(request
    random.cpp
    char_generator.cpp
    request_generator.cpp
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