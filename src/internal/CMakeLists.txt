# Internal module

add_library(internal
    concat.cpp
)

target_include_directories(internal PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(internal
    schema
)
