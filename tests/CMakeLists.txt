# Tests module for eino_cpp

# Ensure GoogleTest is available
find_package(GTest REQUIRED)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${GTEST_INCLUDE_DIRS}
)

# Schema tests
add_executable(stream_copy_test
    schema/stream_copy_test.cpp
)
target_link_libraries(stream_copy_test
    schema
    ${GTEST_BOTH_LIBRARIES}
    pthread
)

# Internal tests
add_executable(concat_test
    internal/concat_test.cpp
)
target_link_libraries(concat_test
    ${GTEST_BOTH_LIBRARIES}
    pthread
)

add_executable(channel_test
    internal/channel_test.cpp
)
target_link_libraries(channel_test
    ${GTEST_BOTH_LIBRARIES}
    pthread
)

# Enable testing
enable_testing()

add_test(NAME stream_copy_test COMMAND stream_copy_test)
add_test(NAME concat_test COMMAND concat_test)
add_test(NAME channel_test COMMAND channel_test)
