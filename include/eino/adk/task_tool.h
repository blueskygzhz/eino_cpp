/*
 * Copyright 2025 CloudWeGo Authors
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

#ifndef EINO_CPP_ADK_TASK_TOOL_H_
#define EINO_CPP_ADK_TASK_TOOL_H_

#include <map>
#include <memory>
#include <string>
#include <vector>
#include <functional>

#include "eino/adk/agent.h"
#include "eino/components/tool.h"

namespace eino {
namespace adk {

// TaskToolDescriptionGenerator generates a description for the task tool
// based on available subagents.
// 对齐 eino/adk/prebuilt/deep/deep.go:55
using TaskToolDescriptionGenerator = 
    std::function<std::string(void* ctx, const std::vector<std::shared_ptr<Agent>>& subAgents)>;

// TaskToolArgument represents the input argument for the task tool.
// 对齐 eino/adk/prebuilt/deep/task_tool.go:137-140
struct TaskToolArgument {
    std::string subagent_type;  // Name of the subagent to invoke
    std::string description;    // Task description to pass to the subagent
};

// TaskTool implements the "task" tool for spawning ephemeral subagents
// to handle complex, isolated tasks.
//
// 对齐 eino/adk/prebuilt/deep/task_tool.go:112-116
//
// This tool allows the main agent to delegate complex, multi-step tasks
// to specialized subagents. Each subagent runs independently and returns
// a single result upon completion.
//
// Lifecycle:
// 1. **Spawn** → Provide clear role, instructions, and expected output
// 2. **Run** → The subagent completes the task autonomously
// 3. **Return** → The subagent provides a single structured result
// 4. **Reconcile** → Incorporate the result into the main thread
class TaskTool : public Tool {
public:
    // Constructor
    // @param sub_agents: Map of subagent name -> AgentTool instances
    // @param sub_agent_slice: Vector of all available subagents (for description generation)
    // @param desc_gen: Optional custom description generator function
    TaskTool(
        void* ctx,
        std::map<std::string, std::shared_ptr<Tool>> sub_agents,
        std::vector<std::shared_ptr<Agent>> sub_agent_slice,
        TaskToolDescriptionGenerator desc_gen = nullptr);

    ~TaskTool() override = default;

    // Tool interface implementation
    ToolInfo Info(void* ctx) const override;
    
    std::string InvokableRun(
        void* ctx, 
        const std::string& arguments_json,
        const std::vector<ToolOption>& opts = {}) override;

private:
    std::map<std::string, std::shared_ptr<Tool>> sub_agents_;
    std::vector<std::shared_ptr<Agent>> sub_agent_slice_;
    TaskToolDescriptionGenerator desc_gen_;

    // Default task tool description generator
    // 对齐 eino/adk/prebuilt/deep/task_tool.go:163-173
    static std::string DefaultTaskToolDescription(
        void* ctx, 
        const std::vector<std::shared_ptr<Agent>>& sub_agents);
};

// NewTaskTool creates a new TaskTool instance.
// 
// 对齐 eino/adk/prebuilt/deep/task_tool.go:56-111
//
// @param ctx: Context
// @param task_tool_desc_gen: Optional custom description generator
// @param sub_agents: List of available subagents
// @param without_general_sub_agent: If true, skip creating the general-purpose subagent
// @param chat_model: Chat model for the general-purpose subagent
// @param instruction: System instruction for the general-purpose subagent
// @param tools_config: Tools configuration for the general-purpose subagent
// @param max_iteration: Maximum iterations for the general-purpose subagent
// @param middlewares: Agent middlewares for the general-purpose subagent
//
// @return: Pair of (TaskTool instance, error)
std::pair<std::shared_ptr<TaskTool>, std::string> NewTaskTool(
    void* ctx,
    TaskToolDescriptionGenerator task_tool_desc_gen,
    const std::vector<std::shared_ptr<Agent>>& sub_agents,
    bool without_general_sub_agent,
    std::shared_ptr<ChatModel> chat_model,
    const std::string& instruction,
    const ToolsConfig& tools_config,
    int max_iteration,
    const std::vector<AgentMiddleware>& middlewares);

// NewTaskToolMiddleware creates an AgentMiddleware that includes the task tool.
//
// 对齐 eino/adk/prebuilt/deep/task_tool.go:34-55
//
// @return: Pair of (AgentMiddleware, error)
std::pair<AgentMiddleware, std::string> NewTaskToolMiddleware(
    void* ctx,
    TaskToolDescriptionGenerator task_tool_desc_gen,
    const std::vector<std::shared_ptr<Agent>>& sub_agents,
    bool without_general_sub_agent,
    std::shared_ptr<ChatModel> chat_model,
    const std::string& instruction,
    const ToolsConfig& tools_config,
    int max_iteration,
    const std::vector<AgentMiddleware>& middlewares);

} // namespace adk
} // namespace eino

#endif // EINO_CPP_ADK_TASK_TOOL_H_
