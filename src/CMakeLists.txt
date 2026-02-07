# Core library target for loadgen functionality
add_library(loadgen-core STATIC
    # Request library sources
    request/random.cpp
    request/char_generator.cpp
    request/request_generator.cpp
    request/acknowledged_counter.cpp
    # Note: zipfian, scrambled_zipfian, and skewed_latest are template classes
    # Their .cpp files are just includes, but we include them for completeness
    request/zipfian_int_distribution.cpp
    request/scrambled_zipfian_int_distribution.cpp
    request/skewed_latest_int_distribution.cpp
    # Types library sources
    types/types.cpp
)

target_link_libraries(loadgen-core PUBLIC 
    toml11::toml11
)

target_compile_features(loadgen-core PUBLIC cxx_std_11)
target_compile_options(loadgen-core PRIVATE 
    -Wall -Wextra -Wpedantic
    $<$<CONFIG:Debug>:-g -O0>
    $<$<CONFIG:Release>:-O2>
)

target_include_directories(loadgen-core PUBLIC 
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/request
    ${CMAKE_SOURCE_DIR}/src/types
)

# Alias for modern CMake consumers/Fetched projects.
add_library(loadgen::core ALIAS loadgen-core)

# Optional: Build gen executable
if(BUILD_LOADGEN_GEN)
    add_executable(gen 
        gen.cpp
    )
    target_link_libraries(gen PRIVATE 
        loadgen-core
    )
    target_compile_features(gen PRIVATE cxx_std_11)
    target_compile_options(gen PRIVATE 
        -Wall -Wextra -Wpedantic
    )
    target_include_directories(gen PRIVATE 
        ${CMAKE_SOURCE_DIR}/src
    )
endif()