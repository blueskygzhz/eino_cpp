# ADK module implementation

set(ADK_SOURCES
    types.cpp
    call_options.cpp
    context.cpp
    checkpoint.cpp
    agent.cpp
    chat_model_agent.cpp
    flow.cpp
    flow_agent.cpp
    runner.cpp
    agent_tool.cpp
    interface.cpp
    workflow.cpp
    utils.cpp
    stream_utils.cpp
    utils_missing.cpp
    executor.cpp
    react.cpp          # NEW: ReAct pattern implementation
    task_tool.cpp      # NEW: TaskTool implementation
    prompts.cpp        # NEW: All prompt templates
    session.cpp        # NEW: Session & RunContext management
)

set(ADK_PREBUILT_SOURCES
    prebuilt/plan_execute.cpp
)

add_library(eino_adk ${ADK_SOURCES} ${ADK_PREBUILT_SOURCES})
target_include_directories(eino_adk PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)

# Link against other modules
target_link_libraries(eino_adk 
    eino_schema
    eino_compose
    eino_components
)

# C++11 standard
set_target_properties(eino_adk PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
)
