# Schema module

add_library(schema
    types.cpp
    message_parser.cpp
    document.cpp
    tool.cpp
    message_concat.cpp
    message_format.cpp
    stream_copy.cpp
)

target_include_directories(schema PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)
