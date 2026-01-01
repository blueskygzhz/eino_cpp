# Examples CMakeLists.txt

# Check if examples directory exists
if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR})
    return()
endif()

# Basic examples (header-only)
add_executable(basic_chain_example basic_chain_example.cpp)
target_include_directories(basic_chain_example PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party
)

add_executable(simple_graph_example simple_graph_example.cpp)
target_include_directories(simple_graph_example PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party
)

add_executable(components_example components_example.cpp)
target_link_libraries(components_example
    eino_components
    eino_compose
    eino_schema
)

add_executable(flow_example flow_example.cpp)
target_link_libraries(flow_example
    eino_flow
    eino_compose
    eino_schema
)

# Advanced examples (header-only)
add_executable(graph_run_example graph_run_example.cpp)
target_include_directories(graph_run_example PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party
)

add_executable(checkpoint_example checkpoint_example.cpp)
target_link_libraries(checkpoint_example
    eino_compose
    eino_adk
    eino_schema
)

add_executable(interrupt_state_example interrupt_state_example.cpp)
target_link_libraries(interrupt_state_example
    eino_compose
    eino_adk
    eino_schema
)

# NEW: Plan-Execute example
add_executable(plan_execute_example plan_execute_example.cpp)
target_link_libraries(plan_execute_example
    eino_adk
    eino_adk_prebuilt
    eino_schema
    eino_compose
)

# NEW: DAG Compose example (header-only, no library dependencies)
add_executable(dag_compose_example dag_compose_example.cpp)
target_include_directories(dag_compose_example PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party
)
# No library linking needed for this example - uses header-only Graph implementation

# NEW: Stream Mode example
add_executable(stream_mode_example stream_mode_example.cpp)
target_link_libraries(stream_mode_example
    eino_compose
    eino_schema
)

# NEW: Stream Mode Simple example (using Lambda)
add_executable(stream_mode_simple stream_mode_simple.cpp)
target_include_directories(stream_mode_simple PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party
)

# NEW: Stream Output Demo - 展示真正的流式输出
add_executable(stream_output_demo stream_output_demo.cpp)
target_include_directories(stream_output_demo PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party
)

# NEW: Graph Stream Pipeline - Graph 流式输出完整示例
add_executable(graph_stream_pipeline graph_stream_pipeline.cpp)
target_include_directories(graph_stream_pipeline PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party
)

# NEW: Graph Output Stream - Graph<Input, StreamReader<Output>> 示例
add_executable(graph_output_stream graph_output_stream.cpp)
target_include_directories(graph_output_stream PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party
)

# NEW: Graph Output Stream Simple - 简化版流式输出示例
add_executable(graph_output_stream_simple graph_output_stream_simple.cpp)
target_include_directories(graph_output_stream_simple PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party
)

# Install examples
install(TARGETS 
    basic_chain_example
    simple_graph_example
    components_example
    flow_example
    graph_run_example
    checkpoint_example
    interrupt_state_example
    plan_execute_example
    dag_compose_example
    stream_mode_example
    stream_mode_simple
    stream_output_demo
    graph_stream_pipeline
    graph_output_stream_simple
    RUNTIME DESTINATION bin/examples
)

# NEW: Branch Node Graph Example
add_executable(branch_node_graph_example branch_node_graph_example.cpp)
target_link_libraries(branch_node_graph_example eino_cpp_static)
target_include_directories(branch_node_graph_example PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party
)

# NEW: Branch Node Simple Example (C++14 compatible)
add_executable(branch_node_simple_example branch_node_simple_example.cpp)
target_include_directories(branch_node_simple_example PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../third_party
)

message(STATUS "Examples configured: 16 executables")
