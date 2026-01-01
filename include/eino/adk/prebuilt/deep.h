/*
 * Copyright 2024 CloudWeGo Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef EINO_CPP_ADK_PREBUILT_DEEP_H_
#define EINO_CPP_ADK_PREBUILT_DEEP_H_

#include "../agent.h"
#include "../chat_model_agent.h"
#include "../../components/tool/tool.h"
#include <memory>
#include <vector>
#include <functional>
#include <string>

namespace eino {
namespace adk {
namespace prebuilt {

// DeepAgent Configuration
// DeepAgent is a sophisticated multi-step reasoning agent that uses:
// - Built-in tools (write_todos)
// - Task tool for subagent delegation
// - ChatModelAgent as the core executor
struct DeepAgentConfig {
    // Name is the identifier for the Deep agent
    std::string name;
    
    // Description provides a brief explanation of the agent's purpose
    std::string description;
    
    // ChatModel is the model used by DeepAgent for reasoning and task execution
    void* chat_model = nullptr;
    
    // Instruction contains the system prompt that guides the agent's behavior
    std::string instruction;
    
    // SubAgents are specialized agents that can be invoked by the agent
    std::vector<std::shared_ptr<Agent>> sub_agents;
    
    // MaxIteration limits the maximum number of reasoning iterations
    int max_iteration = 20;
    
    // WithoutWriteTodos disables the built-in write_todos tool when set to true
    bool without_write_todos = false;
    
    // WithoutGeneralSubAgent disables the general-purpose subagent when set to true
    bool without_general_sub_agent = false;
    
    // TaskToolDescriptionGenerator allows customizing the description for the task tool
    typedef std::function<std::string(void* ctx, const std::vector<std::shared_ptr<Agent>>& agents)> 
        TaskToolDescriptionGenerator;
    TaskToolDescriptionGenerator task_tool_description_generator = nullptr;
};

// TODO represents a task item in the todo list
struct TODO {
    std::string content;
    std::string status;  // pending, in_progress, completed
};

// Session key for storing todos
constexpr const char* kDeepAgentSessionKeyTodos = "deep_agent_session_key_todos";

// Session key for storing general purpose agent
constexpr const char* kDeepAgentGeneralAgentKey = "deep_agent_general_agent";

// NewDeepAgent creates a new Deep agent instance with the provided configuration
// This function initializes built-in tools, creates a task tool for subagent orchestration,
// and returns a fully configured ChatModelAgent ready for execution
std::shared_ptr<Agent> NewDeepAgent(void* ctx, const DeepAgentConfig& config);

// Internal utility functions
std::vector<AgentMiddleware> BuildBuiltinAgentMiddlewares(
    void* ctx, 
    bool without_write_todos);

std::shared_ptr<tool::BaseTool> NewWriteTodosTool(void* ctx);

std::shared_ptr<tool::BaseTool> NewTaskTool(
    void* ctx,
    const std::vector<std::shared_ptr<Agent>>& sub_agents,
    bool without_general_sub_agent,
    const DeepAgentConfig::TaskToolDescriptionGenerator& desc_gen);

}  // namespace prebuilt
}  // namespace adk
}  // namespace eino

#endif  // EINO_CPP_ADK_PREBUILT_DEEP_H_
