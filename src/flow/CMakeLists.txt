# Flow module

add_library(flow
    multi_query_retriever.cpp
    parent_indexer.cpp
    parent_retriever.cpp
    router_retriever.cpp
    react_agent.cpp
)

target_include_directories(flow PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(flow
    components
    compose
    schema
)
