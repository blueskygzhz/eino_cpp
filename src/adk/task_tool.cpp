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

#include "eino/adk/task_tool.h"

#include <sstream>
#include <nlohmann/json.hpp>

#include "eino/adk/agent_tool.h"
#include "eino/adk/chat_model_agent.h"
#include "eino/adk/prompts.h"

namespace eino {
namespace adk {

namespace {
    const std::string kTaskToolName = "task";
    const std::string kGeneralAgentName = "general-purpose";
    const std::string kGeneralAgentDescription = 
        "General-purpose agent for researching complex questions, searching for "
        "files and content, and executing multi-step tasks. When you are searching "
        "for a keyword or file and are not confident that you will find the right "
        "match in the first few tries use this agent to perform the search for you. "
        "This agent has access to all tools as the main agent.";
} // anonymous namespace

// TaskTool implementation

TaskTool::TaskTool(
    void* ctx,
    std::map<std::string, std::shared_ptr<Tool>> sub_agents,
    std::vector<std::shared_ptr<Agent>> sub_agent_slice,
    TaskToolDescriptionGenerator desc_gen)
    : sub_agents_(std::move(sub_agents))
    , sub_agent_slice_(std::move(sub_agent_slice))
    , desc_gen_(desc_gen ? desc_gen : DefaultTaskToolDescription) {
}

ToolInfo TaskTool::Info(void* ctx) const {
    ToolInfo info;
    info.name = kTaskToolName;
    info.desc = desc_gen_(ctx, sub_agent_slice_);
    
    // Parameters: subagent_type, description
    // 对齐 eino/adk/prebuilt/deep/task_tool.go:124-133
    info.params_oneof = nlohmann::json{
        {"type", "object"},
        {"properties", {
            {"subagent_type", {{"type", "string"}}},
            {"description", {{"type", "string"}}}
        }},
        {"required", nlohmann::json::array({"subagent_type", "description"})}
    };
    
    return info;
}

std::string TaskTool::InvokableRun(
    void* ctx, 
    const std::string& arguments_json,
    const std::vector<ToolOption>& opts) {
    
    // Parse input arguments
    // 对齐 eino/adk/prebuilt/deep/task_tool.go:142-160
    try {
        auto args = nlohmann::json::parse(arguments_json);
        
        std::string subagent_type = args["subagent_type"];
        std::string description = args["description"];
        
        // Find the subagent
        auto it = sub_agents_.find(subagent_type);
        if (it == sub_agents_.end()) {
            return "Error: subagent type '" + subagent_type + "' not found";
        }
        
        // Prepare agent tool input
        nlohmann::json agent_input{
            {"request", description}
        };
        
        // Invoke the subagent
        return it->second->InvokableRun(ctx, agent_input.dump(), opts);
        
    } catch (const std::exception& e) {
        return std::string("Error parsing task tool arguments: ") + e.what();
    }
}

std::string TaskTool::DefaultTaskToolDescription(
    void* ctx, 
    const std::vector<std::shared_ptr<Agent>>& sub_agents) {
    
    // 对齐 eino/adk/prebuilt/deep/task_tool.go:163-173
    std::ostringstream sub_agents_desc;
    for (const auto& agent : sub_agents) {
        sub_agents_desc << "- " << agent->Name(ctx) << ": " 
                        << agent->Description(ctx) << "\n";
    }
    
    // Use the task tool description template from prompts
    // 对齐 eino/adk/prebuilt/deep/prompt.go:76-179
    std::string desc = prompts::kTaskToolDescription;
    
    // Replace {other_agents} placeholder
    size_t pos = desc.find("{other_agents}");
    if (pos != std::string::npos) {
        desc.replace(pos, 14, sub_agents_desc.str());
    }
    
    return desc;
}

// NewTaskTool implementation

std::pair<std::shared_ptr<TaskTool>, std::string> NewTaskTool(
    void* ctx,
    TaskToolDescriptionGenerator task_tool_desc_gen,
    const std::vector<std::shared_ptr<Agent>>& sub_agents,
    bool without_general_sub_agent,
    std::shared_ptr<ChatModel> chat_model,
    const std::string& instruction,
    const ToolsConfig& tools_config,
    int max_iteration,
    const std::vector<AgentMiddleware>& middlewares) {
    
    // 对齐 eino/adk/prebuilt/deep/task_tool.go:56-111
    
    std::map<std::string, std::shared_ptr<Tool>> sub_agent_tools;
    std::vector<std::shared_ptr<Agent>> sub_agent_slice = sub_agents;
    
    // Create general-purpose agent if needed
    if (!without_general_sub_agent) {
        ChatModelAgentConfig config;
        config.name = kGeneralAgentName;
        config.description = kGeneralAgentDescription;
        config.instruction = instruction;
        config.model = chat_model;
        config.tools_config = tools_config;
        config.max_iterations = max_iteration;
        config.middlewares = middlewares;
        
        auto general_agent = NewChatModelAgent(ctx, config);
        if (!general_agent) {
            return {nullptr, "Failed to create general-purpose agent"};
        }
        
        // Convert to AgentTool
        auto agent_tool = NewAgentTool(ctx, general_agent);
        if (!agent_tool) {
            return {nullptr, "Failed to create AgentTool for general agent"};
        }
        
        sub_agent_tools[kGeneralAgentName] = agent_tool;
        sub_agent_slice.push_back(general_agent);
    }
    
    // Convert all subagents to tools
    for (const auto& agent : sub_agents) {
        std::string name = agent->Name(ctx);
        
        auto agent_tool = NewAgentTool(ctx, agent);
        if (!agent_tool) {
            return {nullptr, "Failed to create AgentTool for " + name};
        }
        
        sub_agent_tools[name] = agent_tool;
    }
    
    auto task_tool = std::make_shared<TaskTool>(
        ctx, 
        sub_agent_tools, 
        sub_agent_slice,
        task_tool_desc_gen);
    
    return {task_tool, ""};
}

std::pair<AgentMiddleware, std::string> NewTaskToolMiddleware(
    void* ctx,
    TaskToolDescriptionGenerator task_tool_desc_gen,
    const std::vector<std::shared_ptr<Agent>>& sub_agents,
    bool without_general_sub_agent,
    std::shared_ptr<ChatModel> chat_model,
    const std::string& instruction,
    const ToolsConfig& tools_config,
    int max_iteration,
    const std::vector<AgentMiddleware>& middlewares) {
    
    // 对齐 eino/adk/prebuilt/deep/task_tool.go:34-55
    
    auto [task_tool, err] = NewTaskTool(
        ctx,
        task_tool_desc_gen,
        sub_agents,
        without_general_sub_agent,
        chat_model,
        instruction,
        tools_config,
        max_iteration,
        middlewares);
    
    if (!err.empty()) {
        return {AgentMiddleware{}, err};
    }
    
    AgentMiddleware middleware;
    middleware.additional_instruction = prompts::kTaskPrompt;
    middleware.additional_tools = {task_tool};
    
    return {middleware, ""};
}

} // namespace adk
} // namespace eino
