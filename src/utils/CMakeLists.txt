# Utils module

add_library(utils
    callbacks_template.cpp
)

target_include_directories(utils PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(utils
    callbacks
    components
)
