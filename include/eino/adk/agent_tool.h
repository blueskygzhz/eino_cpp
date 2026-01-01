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

#ifndef EINO_CPP_ADK_AGENT_TOOL_H_
#define EINO_CPP_ADK_AGENT_TOOL_H_

// AgentTool - Wraps an Agent as a Tool
// ====================================
// Enables agents to be used as tools within compose graphs and other agent executions.
// Aligns with eino adk.AgentTool implementation.
//
// EXECUTION LIFECYCLE:
// 1. Check for interrupt/resume state in compose state
// 2. Prepare input messages based on options:
//    - full_chat_history_as_input: use chat history from state
//    - Otherwise: parse JSON arguments and extract "request" field
// 3. Create internal Runner with checkpoint store
// 4. Execute agent and iterate events:
//    - Forward output events to caller
//    - Detect interrupt events and save checkpoint
// 5. Return result or InterruptAndRerun signal

#include "agent.h"
#include "types.h"
#include "runner.h"
#include "async_iterator.h"
#include "../components/tool.h"
#include "../schema/types.h"

#include <memory>
#include <vector>
#include <functional>
#include <string>

namespace eino {
namespace adk {

// AgentToolOptions configuration for AgentTool
// Aligns with eino adk.AgentToolOptions structure
struct AgentToolOptions {
    // If true, pass complete chat history to agent
    // If false, parse JSON and extract "request" field as single user message
    bool full_chat_history_as_input = false;
    
    // Custom input schema for the tool
    // If null, uses default schema with "request" parameter
    std::shared_ptr<void> agent_input_schema;  // schema::ParamsOneOf*
};

// AgentToolOption is an option function for AgentTool
// Aligns with eino adk.AgentToolOption pattern
typedef std::function<void(std::shared_ptr<AgentToolOptions>&)> AgentToolOption;

// WithFullChatHistoryAsInput enables using full chat history as agent input
// Aligns with eino adk.WithFullChatHistoryAsInput()
AgentToolOption WithFullChatHistoryAsInput();

// WithAgentInputSchema sets a custom input schema for the agent
// Aligns with eino adk.WithAgentInputSchema(schema)
AgentToolOption WithAgentInputSchema(std::shared_ptr<void> schema);

// Internal structure for storing interrupt checkpoint
struct AgentToolInterruptInfo {
    std::shared_ptr<AgentEvent> last_event;
    std::string checkpoint_data;
};

// AgentTool wraps an Agent as an InvokableTool for use in compose graphs
// Aligns with eino adk.NewAgentTool implementation
// Key features:
// - Supports interrupt/resume via compose State checkpoint mechanism
// - Automatic input preparation from JSON or chat history
// - Full event iteration with interrupt detection
// - Checkpoint persistence for recovery
class AgentTool : public components::BaseTool {
public:
    // Constructs an AgentTool wrapping the given agent
    explicit AgentTool(
        void* ctx,
        const std::shared_ptr<Agent>& agent,
        const std::shared_ptr<AgentToolOptions>& options = nullptr);
    
    virtual ~AgentTool() = default;
    
    // Info returns the tool information (name, description, parameter schema)
    // Aligns with eino adk.agentTool.Info(ctx)
    schema::ToolInfo Info(void* ctx) override;
    
    // InvokableRun executes the tool with JSON arguments
    // Implements the complete execution lifecycle:
    // - Check for interrupt/resume state
    // - Prepare input messages
    // - Execute agent and iterate events
    // - Handle interrupts by saving checkpoint
    // - Return result or throw InterruptAndRerun
    // Aligns with eino adk.agentTool.InvokableRun(ctx, argumentsInJSON, opts)
    std::string InvokableRun(
        void* ctx,
        const std::string& arguments_in_json,
        const std::vector<std::shared_ptr<components::Option>>& opts
            = std::vector<std::shared_ptr<components::Option>>()) override;

private:
    std::shared_ptr<Agent> agent_;
    std::shared_ptr<AgentToolOptions> options_;
    
    // PrepareInput prepares input messages based on configuration
    // If full_chat_history_as_input: get chat history from compose state
    // Otherwise: parse JSON and extract "request" field
    std::vector<std::shared_ptr<schema::Message>> PrepareInput(
        void* ctx,
        const std::string& arguments_in_json);
    
    // GetReactChatHistory gets chat history from compose State for React pattern
    // Aligns with eino adk.getReactChatHistory
    // Returns messages from State with transfer messages appended
    std::vector<std::shared_ptr<schema::Message>> GetReactChatHistory(
        void* ctx,
        const std::string& dest_agent_name);
    
    // RewriteMessage rewrites message with agent name prefix
    // Aligns with eino adk.rewriteMessage
    std::shared_ptr<schema::Message> RewriteMessage(
        const std::shared_ptr<schema::Message>& msg,
        const std::string& agent_name);
    
    // ExtractOutput extracts content from agent output event
    // Handles both streaming and non-streaming output
    std::string ExtractOutput(const std::shared_ptr<AgentEvent>& event);
    
    // ExtractRequestField extracts "request" field from JSON string
    std::string ExtractRequestField(const std::string& json_str);
};

// NewAgentTool creates a new AgentTool wrapping the given agent
// Aligns with eino adk.NewAgentTool(ctx, agent, options...)
// Returns a BaseTool that can be used in compose graphs
std::shared_ptr<components::BaseTool> NewAgentTool(
    void* ctx,
    const std::shared_ptr<Agent>& agent,
    const std::vector<AgentToolOption>& options = {});

// Convenience overload for single option
std::shared_ptr<components::BaseTool> NewAgentTool(
    void* ctx,
    const std::shared_ptr<Agent>& agent,
    const AgentToolOption& option);

}  // namespace adk
}  // namespace eino

#endif  // EINO_CPP_ADK_AGENT_TOOL_H_
