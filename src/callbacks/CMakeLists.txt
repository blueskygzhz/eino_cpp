# Callbacks module sources
set(CALLBACKS_SOURCES
    handler_builder.cpp
    manager.cpp
    callback.cpp
)

target_sources(eino_cpp_static PRIVATE
    ${CALLBACKS_SOURCES}
)
