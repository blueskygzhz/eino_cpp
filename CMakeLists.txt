cmake_minimum_required(VERSION 3.10)
project(eino_cpp VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/third_party)

# Collect all header files for library
file(GLOB_RECURSE EINO_HEADERS 
    "include/eino/*.h"
    "include/eino/**/*.h"
)

# Create header-only library
add_library(eino_cpp INTERFACE)
target_include_directories(eino_cpp INTERFACE 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Collect all source files
set(EINO_SOURCES
    # Callbacks
    src/callbacks/handler_builder.cpp
    
    # ADK sources
    src/adk/agent.cpp
    src/adk/agent_tool.cpp
    src/adk/call_options.cpp
    src/adk/chat_model_agent.cpp
    src/adk/checkpoint.cpp
    src/adk/context.cpp
    src/adk/deterministic_transfer.cpp
    src/adk/flow.cpp
    src/adk/flow_agent.cpp
    src/adk/interface.cpp
    src/adk/runner.cpp
    src/adk/types.cpp
    src/adk/instruction.cpp
    src/adk/interrupt.cpp
    src/adk/runctx.cpp
    src/adk/utils.cpp
    src/adk/executor.cpp
    src/adk/workflow.cpp
    src/adk/prebuilt/deep.cpp
    src/adk/prebuilt/plan_execute.cpp
    src/adk/prebuilt/react.cpp
    src/adk/prebuilt/supervisor.cpp
    
    # Compose sources
    src/compose/branch.cpp
    src/compose/chain.cpp
    src/compose/chain_branch.cpp
    src/compose/chain_parallel.cpp
    src/compose/checkpoint.cpp
    src/compose/component_to_graph_node.cpp
    src/compose/dag.cpp
    src/compose/error.cpp
    src/compose/field_mapping.cpp
    src/compose/generic_graph.cpp
    src/compose/generic_helper.cpp
    src/compose/graph.cpp
    src/compose/graph_add_node_options.cpp
    src/compose/graph_call_options.cpp
    src/compose/graph_compile_options.cpp
    src/compose/graph_manager.cpp
    src/compose/graph_node.cpp
    src/compose/graph_run.cpp
    src/compose/introspect.cpp
    src/compose/pregel.cpp
    src/compose/runnable.cpp
    src/compose/state.cpp
    src/compose/stream_concat.cpp
    src/compose/stream_reader.cpp
    src/compose/tool_node.cpp
    src/compose/types_lambda.cpp
    src/compose/utils.cpp
    src/compose/values_merge.cpp
    src/compose/workflow.cpp
    
    # Components sources
    src/components/interface.cpp
    src/components/prompt.cpp
    src/components/simple_embedder.cpp
    src/components/simple_loader.cpp
    src/components/text_splitter.cpp
    
    # Flow sources
    src/flow/multi_query_retriever.cpp
    src/flow/parent_indexer.cpp
    src/flow/parent_retriever.cpp
    src/flow/router_retriever.cpp
    src/flow/react_agent.cpp
    
    # Schema sources
    src/schema/types.cpp
    src/schema/message_parser.cpp
    src/schema/document.cpp
    src/schema/tool.cpp
    
    # Internal sources
    src/internal/concat.cpp
    
    # Utils sources
    src/utils/callbacks_template.cpp
)

# Create static library
add_library(eino_cpp_static STATIC ${EINO_SOURCES})
target_include_directories(eino_cpp_static PRIVATE 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_include_directories(eino_cpp_static PUBLIC
    $<INSTALL_INTERFACE:include>
)
set_target_properties(eino_cpp_static PROPERTIES
    OUTPUT_NAME eino_cpp
    CXX_STANDARD 14
)

# Examples subdirectory
add_subdirectory(examples)

# Enable testing (optional - skip if GTest not found)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests AND NOT DEFINED SKIP_TESTS)
    find_package(GTest QUIET)
    if(GTest_FOUND OR GTEST_FOUND)
        enable_testing()
        add_subdirectory(tests)
        message(STATUS "Tests enabled")
    else()
        message(STATUS "GTest not found - tests disabled (set -DSKIP_TESTS=OFF to require tests)")
    endif()
endif()

# Install targets
install(TARGETS eino_cpp EXPORT eino_cpp_targets)
install(TARGETS eino_cpp_static EXPORT eino_cpp_targets)
install(EXPORT eino_cpp_targets DESTINATION lib/cmake/eino_cpp)
install(DIRECTORY include/eino DESTINATION include)
