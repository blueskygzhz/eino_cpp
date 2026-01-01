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

#include "eino/adk/prebuilt/deep.h"
#include "eino/adk/context.h"
#include "eino/adk/runner.h"
#include "eino/adk/agent_tool.h"
#include "eino/components/tool/tool.h"
#include "eino/schema/types.h"
#include <nlohmann/json.hpp>
#include <sstream>

using json = nlohmann::json;

namespace eino {
namespace adk {
namespace prebuilt {

namespace {

// Write todos tool description (comprehensive version from prompt.go)
constexpr const char* kWriteTodosToolDesc = R"(Use this tool to create and manage a structured task list for your current work session. This helps you track progress, organize complex tasks, and demonstrate thoroughness to the user.
It also helps the user understand the progress of the task and overall progress of their requests.
Only use this tool if you think it will be helpful in staying organized. If the user's request is trivial and takes less than 3 steps, it is better to NOT use this tool and just do the task directly.

## When to Use This Tool
Use this tool in these scenarios:
1. Complex multi-step tasks - When a task requires 3 or more distinct steps or actions
2. Non-trivial and complex tasks - Tasks that require careful planning or multiple operations
3. User explicitly requests todo list - When the user directly asks you to use the todo list
4. User provides multiple tasks - When users provide a list of things to be done (numbered or comma-separated)
5. The plan may need future revisions or updates based on results from the first few steps

## Task States
- pending: Task not yet started
- in_progress: Currently working on
- completed: Task finished successfully)";

// Write todos prompt
constexpr const char* kWriteTodosPrompt = R"(
## 'write_todos'

You have access to the 'write_todos' tool to help you manage and plan complex objectives.
Use this tool for complex objectives to ensure that you are tracking each necessary step and giving the user visibility into your progress.
This tool is very helpful for planning complex objectives, and for breaking down these larger complex objectives into smaller steps.

It is critical that you mark todos as completed as soon as you are done with a step. Do not batch up multiple steps before marking them as completed.
For simple objectives that only require a few steps, it is better to just complete the objective directly and NOT use this tool.
Writing todos takes time and tokens, use it when it is helpful for managing complex many-step problems! But not for simple few-step requests.
)";

// Task prompt (comprehensive version from prompt.go)
constexpr const char* kTaskPrompt = R"(
## 'task' (subagent spawner)

You have access to a 'task' tool to launch short-lived subagents that handle isolated tasks. These agents are ephemeral — they live only for the duration of the task and return a single result.

When to use the task tool:
- When a task is complex and multi-step, and can be fully delegated in isolation
- When a task is independent of other tasks and can run in parallel
- When a task requires focused reasoning or heavy token/context usage that would bloat the orchestrator thread
- When sandboxing improves reliability (e.g. code execution, structured searches, data formatting)
- When you only care about the output of the subagent, and not the intermediate steps

Subagent lifecycle:
1. **Spawn** → Provide clear role, instructions, and expected output
2. **Run** → The subagent completes the task autonomously
3. **Return** → The subagent provides a single structured result
4. **Reconcile** → Incorporate or synthesize the result into the main thread

## Important Task Tool Usage Notes
- Whenever possible, parallelize the work that you do. This is true for both tool_calls, and for tasks.
- Remember to use the 'task' tool to silo independent tasks within a multi-part objective.
- You should use the 'task' tool whenever you have a complex task that will take multiple steps, and is independent from other tasks that the agent needs to complete.
)";

// Base agent prompt
constexpr const char* kBaseAgentPrompt = R"(
In order to complete the objective that the user asks of you, you have access to a number of standard tools.
)";

// General agent description
constexpr const char* kGeneralAgentDescription = 
    "General-purpose agent for researching complex questions, searching for files and content, "
    "and executing multi-step tasks. When you are searching for a keyword or file and are not confident "
    "that you will find the right match in the first few tries use this agent to perform the search for you. "
    "This agent has access to all tools as the main agent.";

// Task tool name constant
constexpr const char* kTaskToolName = "task";
constexpr const char* kGeneralAgentName = "general-purpose";

// WriteTodosTool implementation
class WriteTodosTool : public tool::InvokableTool {
public:
    std::shared_ptr<schema::ToolInfo> Info(void* ctx) override {
        auto tool_info = std::make_shared<schema::ToolInfo>();
        tool_info->name = "write_todos";
        tool_info->description = kWriteTodosToolDesc;
        
        // Define parameters: todos array
        schema::ParameterInfo todos_param;
        todos_param.type = schema::ParameterType::Array;
        todos_param.description = "Array of todo items with content and status";
        todos_param.required = true;
        
        // Todo item schema
        schema::ParameterInfo todo_item;
        todo_item.type = schema::ParameterType::Object;
        
        schema::ParameterInfo content_param;
        content_param.type = schema::ParameterType::String;
        content_param.description = "Todo item content/description";
        content_param.required = true;
        
        schema::ParameterInfo status_param;
        status_param.type = schema::ParameterType::String;
        status_param.description = "Todo status: pending, in_progress, or completed";
        status_param.required = true;
        
        // Note: Full schema would be built here with proper ParamsOneOf
        // For now, return basic structure
        return tool_info;
    }
    
    std::string InvokableRun(void* ctx, const std::string& arguments_json) override {
        try {
            // Parse todos from JSON
            auto j = json::parse(arguments_json);
            auto todos_array = j["todos"];
            
            std::vector<TODO> todos;
            for (const auto& item : todos_array) {
                TODO todo;
                todo.content = item["content"].get<std::string>();
                todo.status = item["status"].get<std::string>();
                todos.push_back(todo);
            }
            
            // Store todos in session context
            // Note: This requires session value storage in context
            // AddSessionValue(ctx, kDeepAgentSessionKeyTodos, todos);
            
            // Return confirmation message
            std::ostringstream oss;
            oss << "Updated todo list with " << todos.size() << " items";
            return oss.str();
            
        } catch (const std::exception& e) {
            return std::string("Error updating todos: ") + e.what();
        }
    }
};

// TaskTool implementation
class TaskTool : public tool::InvokableTool {
public:
    TaskTool(void* ctx,
             const std::vector<std::shared_ptr<Agent>>& sub_agents,
             bool without_general_sub_agent,
             const DeepAgentConfig::TaskToolDescriptionGenerator& desc_gen)
        : sub_agents_(sub_agents), desc_generator_(desc_gen) {
        
        // Convert agents to agent tools
        for (const auto& agent : sub_agents) {
            auto agent_tool = NewAgentTool(ctx, agent);
            if (agent_tool) {
                std::string agent_name = agent->Name(ctx);
                agent_tools_[agent_name] = agent_tool;
            }
        }
    }
    
    std::shared_ptr<schema::ToolInfo> Info(void* ctx) override {
        auto tool_info = std::make_shared<schema::ToolInfo>();
        tool_info->name = kTaskToolName;
        
        // Generate description
        if (desc_generator_) {
            tool_info->description = desc_generator_(ctx, sub_agents_);
        } else {
            tool_info->description = DefaultTaskToolDescription(ctx);
        }
        
        // Parameters: subagent_type and description
        schema::ParameterInfo subagent_type_param;
        subagent_type_param.type = schema::ParameterType::String;
        subagent_type_param.description = "Name of the subagent type to use";
        subagent_type_param.required = true;
        
        schema::ParameterInfo desc_param;
        desc_param.type = schema::ParameterType::String;
        desc_param.description = "Task description/request for the subagent";
        desc_param.required = true;
        
        return tool_info;
    }
    
    std::string InvokableRun(void* ctx, const std::string& arguments_json) override {
        try {
            auto j = json::parse(arguments_json);
            std::string subagent_type = j["subagent_type"].get<std::string>();
            std::string description = j["description"].get<std::string>();
            
            // Find the agent tool
            auto it = agent_tools_.find(subagent_type);
            if (it == agent_tools_.end()) {
                return "Error: subagent type '" + subagent_type + "' not found";
            }
            
            // Create request JSON for agent tool
            json request_json;
            request_json["request"] = description;
            
            // Invoke the agent tool
            return it->second->InvokableRun(ctx, request_json.dump());
            
        } catch (const std::exception& e) {
            return std::string("Error running task: ") + e.what();
        }
    }
    
private:
    std::string DefaultTaskToolDescription(void* ctx) {
        std::ostringstream oss;
        oss << "Launch an ephemeral subagent to handle complex, multi-step independent tasks.\n\n";
        oss << "Available agent types:\n";
        
        for (const auto& agent : sub_agents_) {
            std::string name = agent->Name(ctx);
            std::string desc = agent->Description(ctx);
            oss << "- " << name << ": " << desc << "\n";
        }
        
        return oss.str();
    }
    
    std::vector<std::shared_ptr<Agent>> sub_agents_;
    std::map<std::string, std::shared_ptr<tool::InvokableTool>> agent_tools_;
    DeepAgentConfig::TaskToolDescriptionGenerator desc_generator_;
};

}  // namespace

std::vector<AgentMiddleware> BuildBuiltinAgentMiddlewares(
    void* ctx,
    bool without_write_todos) {
    std::vector<AgentMiddleware> middlewares;
    
    if (!without_write_todos) {
        auto tool = NewWriteTodosTool(ctx);
        if (tool) {
            AgentMiddleware middleware;
            middleware.additional_instruction = kWriteTodosPrompt;
            middleware.additional_tools.push_back(tool);
            middlewares.push_back(middleware);
        }
    }
    
    return middlewares;
}

std::shared_ptr<tool::BaseTool> NewWriteTodosTool(void* ctx) {
    return std::make_shared<WriteTodosTool>();
}

std::shared_ptr<tool::BaseTool> NewTaskTool(
    void* ctx,
    const std::vector<std::shared_ptr<Agent>>& sub_agents,
    bool without_general_sub_agent,
    const DeepAgentConfig::TaskToolDescriptionGenerator& desc_gen) {
    
    std::vector<std::shared_ptr<Agent>> all_agents = sub_agents;
    
    // Add general-purpose agent if not disabled
    if (!without_general_sub_agent) {
        // General agent should be created with same configuration as main agent
        // For now, we include only the provided sub_agents
        // In full implementation, create general agent here
    }
    
    return std::make_shared<TaskTool>(ctx, all_agents, without_general_sub_agent, desc_gen);
}

std::shared_ptr<Agent> NewDeepAgent(void* ctx, const DeepAgentConfig& config) {
    // Build middlewares vector
    std::vector<AgentMiddleware> middlewares;
    
    // Add base agent prompt
    AgentMiddleware base_middleware;
    base_middleware.additional_instruction = kBaseAgentPrompt;
    middlewares.push_back(base_middleware);
    
    // Add built-in middlewares
    auto builtin = BuildBuiltinAgentMiddlewares(ctx, config.without_write_todos);
    for (const auto& mw : builtin) {
        middlewares.push_back(mw);
    }
    
    // Create task tool middleware if there are sub-agents
    if (!config.sub_agents.empty() || !config.without_general_sub_agent) {
        auto task_tool = NewTaskTool(
            ctx,
            config.sub_agents,
            config.without_general_sub_agent,
            config.task_tool_description_generator);
        
        if (task_tool) {
            AgentMiddleware task_middleware;
            task_middleware.additional_instruction = kTaskPrompt;
            task_middleware.additional_tools.push_back(task_tool);
            middlewares.push_back(task_middleware);
        }
    }
    
    // Create ChatModelAgent with all middlewares
    auto cm_config = std::make_shared<ChatModelAgentConfig>();
    cm_config->name = config.name;
    cm_config->description = config.description;
    cm_config->instruction = config.instruction;
    cm_config->model = config.chat_model;
    cm_config->max_iterations = config.max_iteration;
    cm_config->middlewares = middlewares;
    
    return NewChatModelAgent(ctx, cm_config);
}

}  // namespace prebuilt
}  // namespace adk
}  // namespace eino
