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

#include "eino/adk/executor.h"
#include "eino/adk/chat_model_agent.h"
#include <sstream>
#include <algorithm>
#include <stdexcept>

namespace eino {
namespace adk {

// ============================================================================
// Helper Functions for Executor
// ============================================================================

// formatInput formats input messages into a string
static std::string formatInput(const std::vector<Message>& input) {
    std::ostringstream ss;
    for (const auto& msg : input) {
        ss << msg.content << "\n";
    }
    return ss.str();
}

// formatExecutedSteps formats executed steps into a string for context
static std::string formatExecutedSteps(
    const std::vector<std::pair<std::string, std::string>>& executed_steps) {
    std::ostringstream ss;
    for (const auto& [step, result] : executed_steps) {
        ss << "Step: " << step << "\n";
        ss << "Result: " << result << "\n\n";
    }
    return ss.str();
}

// defaultGenExecutorInputFn is the default input generator for executor
// It formats the execution context into a structured prompt
static std::vector<Message> defaultGenExecutorInputFn(
    void* ctx, const ExecutionContext& exec_ctx) {
    
    std::vector<Message> messages;
    
    // System message: instruct the executor how to behave
    Message system_msg;
    system_msg.role = "system";
    system_msg.content = 
        "You are a diligent and meticulous executor agent. "
        "Follow the given plan and execute your tasks carefully and thoroughly.";
    messages.push_back(system_msg);
    
    // User message: provide execution context and current task
    std::ostringstream user_content;
    user_content << "## OBJECTIVE\n";
    user_content << formatInput(exec_ctx.user_input);
    user_content << "\n## Given the following plan:\n";
    user_content << exec_ctx.plan;
    user_content << "\n## COMPLETED STEPS & RESULTS\n";
    user_content << formatExecutedSteps(exec_ctx.executed_steps);
    user_content << "\n## Your task is to execute the first step";
    
    Message user_msg;
    user_msg.role = "user";
    user_msg.content = user_content.str();
    messages.push_back(user_msg);
    
    return messages;
}

// ============================================================================
// Executor Implementation
// ============================================================================

Executor::Executor(const ExecutorConfig& config) : config_(config) {
    if (!config_.model) {
        throw std::invalid_argument("ExecutorConfig requires a valid ChatModel");
    }
    
    // Default values for optional fields
    if (config_.name.empty()) {
        config_.name = "Executor";
    }
    if (config_.description.empty()) {
        config_.description = "an executor agent";
    }
    if (config_.max_iterations <= 0) {
        config_.max_iterations = 20;
    }
}

Executor::~Executor() = default;

std::string Executor::Name(void* ctx) {
    return config_.name;
}

std::string Executor::Description(void* ctx) {
    return config_.description;
}

std::shared_ptr<AsyncIterator<std::shared_ptr<AgentEvent>>> Executor::Run(
    void* ctx,
    const std::shared_ptr<AgentInput>& input,
    const std::vector<std::shared_ptr<AgentRunOption>>& options) {
    
    // Align with eino: Executor wraps ChatModelAgent
    // It creates a ChatModelAgent configured with tools and the execution context
    
    // Get the input generator function (use default if not provided)
    GenExecutorInputFn gen_input_fn = config_.gen_input_fn 
        ? config_.gen_input_fn 
        : defaultGenExecutorInputFn;
    
    // Create the wrapper function for ChatModelAgent's GenModelInput
    // This closure captures session values and builds the execution context
    auto chat_agent_gen_input = [this, gen_input_fn](
        void* ctx,
        const std::string& instruction,
        const std::shared_ptr<AgentInput>& agent_input) -> std::vector<Message> {
        
        // CRITICAL: Retrieve session values set by previous agents (Planner, etc)
        // In eino, these are retrieved via adk.GetSessionValue()
        
        // For now, we use a simplified approach:
        // Get the plan from session (set by Planner)
        auto plan_ptr = GetSessionValue(ctx, kExecutorSessionKeyPlan);
        if (!plan_ptr) {
            throw std::runtime_error(
                "Executor requires Plan in session (should be set by Planner)");
        }
        std::string plan = *(reinterpret_cast<std::string*>(plan_ptr));
        
        // Get user input from session
        auto user_input_ptr = GetSessionValue(ctx, kExecutorSessionKeyUserInput);
        if (!user_input_ptr) {
            throw std::runtime_error(
                "Executor requires UserInput in session");
        }
        std::vector<Message> user_input = 
            *(reinterpret_cast<std::vector<Message>*>(user_input_ptr));
        
        // Get executed steps from session (optional, may not exist yet)
        std::vector<std::pair<std::string, std::string>> executed_steps;
        auto executed_steps_ptr = GetSessionValue(ctx, kExecutorSessionKeyExecutedSteps);
        if (executed_steps_ptr) {
            executed_steps = 
                *(reinterpret_cast<std::vector<std::pair<std::string, std::string>>*>(
                    executed_steps_ptr));
        }
        
        // Build execution context
        ExecutionContext exec_ctx;
        exec_ctx.user_input = user_input;
        exec_ctx.plan = plan;
        exec_ctx.executed_steps = executed_steps;
        
        // Generate input messages for this execution phase
        std::vector<Message> messages = gen_input_fn(ctx, exec_ctx);
        
        return messages;
    };
    
    // Build ChatModelAgent configuration
    ChatModelAgentConfig chat_cfg;
    chat_cfg.name = config_.name;
    chat_cfg.description = config_.description;
    chat_cfg.model = config_.model;
    chat_cfg.tools_config = config_.tools_config;
    chat_cfg.max_iterations = config_.max_iterations;
    chat_cfg.gen_model_input = chat_agent_gen_input;
    chat_cfg.output_key = kExecutorSessionKeyResult;  // Store result in session
    
    // Create ChatModelAgent (the actual executor is a ChatModelAgent with tools)
    auto chat_agent = std::make_shared<ChatModelAgent>(chat_cfg);
    
    // Delegate to ChatModelAgent::Run
    return chat_agent->Run(ctx, input, options);
}

void Executor::AddTool(const std::shared_ptr<Tool>& tool) {
    if (!tool) return;
    config_.tools_config.tools.push_back(tool);
}

void Executor::RemoveTool(const std::string& tool_name) {
    // Remove tool by matching name from ToolInfo
    // Aligns with Go version: tools are identified by name
    auto it = std::remove_if(
        config_.tools_config.tools.begin(),
        config_.tools_config.tools.end(),
        [&tool_name, this](const std::shared_ptr<Tool>& tool) {
            if (!tool) {
                return false;
            }
            // Get tool info to access name
            // Note: ctx should ideally be passed in, using nullptr for now
            auto tool_info = tool->GetToolInfo(nullptr);
            return tool_info.name == tool_name;
        }
    );
    
    if (it != config_.tools_config.tools.end()) {
        config_.tools_config.tools.erase(it, config_.tools_config.tools.end());
    }
}

std::shared_ptr<Tool> Executor::GetTool(const std::string& tool_name) {
    // Find tool by matching name from ToolInfo
    // Aligns with Go version: tools are identified by name
    for (const auto& tool : config_.tools_config.tools) {
        if (!tool) {
            continue;
        }
        // Get tool info to access name
        // Note: ctx should ideally be passed in, using nullptr for now
        auto tool_info = tool->GetToolInfo(nullptr);
        if (tool_info.name == tool_name) {
            return tool;
        }
    }
    return nullptr;
}

// ============================================================================
// Factory Function
// ============================================================================

std::shared_ptr<Executor> NewExecutor(void* ctx, const ExecutorConfig& config) {
    // Direct parallel to eino's NewExecutor
    // In eino, NewExecutor creates a ChatModelAgent configured as executor
    // We do the same here
    return std::make_shared<Executor>(config);
}

// ============================================================================
// Session Management Functions
// ============================================================================

// GetSessionValue retrieves a value from the execution context/session
// This is a simplified implementation - in production it would interface with
// the eino context management system
void* GetSessionValue(void* ctx, const std::string& key) {
    // Placeholder: In real implementation, this would:
    // 1. Cast ctx to the appropriate context type
    // 2. Query the session/context for the value
    // 3. Return the value pointer or nullptr if not found
    
    // For now, returning nullptr indicates not implemented
    return nullptr;
}

// AddSessionValue adds a value to the execution context/session
// This is a simplified implementation - in production it would interface with
// the eino context management system
void AddSessionValue(void* ctx, const std::string& key, void* value) {
    // Placeholder: In real implementation, this would:
    // 1. Cast ctx to the appropriate context type
    // 2. Store the value in the session/context
    // 3. Handle lifecycle/cleanup as needed
}

}  // namespace adk
}  // namespace eino
