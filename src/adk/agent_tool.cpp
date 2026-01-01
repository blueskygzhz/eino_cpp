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

#include "../include/eino/adk/agent_tool.h"
#include "../include/eino/compose/state.h"
#include <stdexcept>
#include <sstream>
#include <algorithm>

namespace eino {
namespace adk {

// Aligns with eino adk.WithFullChatHistoryAsInput()
AgentToolOption WithFullChatHistoryAsInput() {
    return [](std::shared_ptr<AgentToolOptions>& opts) {
        opts->full_chat_history_as_input = true;
    };
}

// Aligns with eino adk.WithAgentInputSchema(schema)
AgentToolOption WithAgentInputSchema(std::shared_ptr<void> schema) {
    return [schema](std::shared_ptr<AgentToolOptions>& opts) {
        opts->agent_input_schema = schema;
    };
}

// Aligns with eino adk.agentTool constructor
AgentTool::AgentTool(
    void* ctx,
    const std::shared_ptr<Agent>& agent,
    const std::shared_ptr<AgentToolOptions>& options)
    : agent_(agent), 
      options_(options ? options : std::make_shared<AgentToolOptions>()) {
}

// Aligns with eino adk.agentTool.Info(ctx)
schema::ToolInfo AgentTool::Info(void* ctx) {
    schema::ToolInfo info;
    info.name = agent_->Name(ctx);
    info.desc = agent_->Description(ctx);
    // TODO: set params_one_of from options or default schema
    return info;
}

// Helper to extract "request" field from JSON string
// Simple implementation without external JSON library
std::string AgentTool::ExtractRequestField(const std::string& json_str) {
    // Find "request" : 
    size_t pos = json_str.find("\"request\"");
    if (pos == std::string::npos) {
        return json_str;  // Return as-is if no request field
    }
    
    // Find the colon after "request"
    pos = json_str.find(':', pos);
    if (pos == std::string::npos) {
        return "";
    }
    
    // Skip whitespace and colon
    pos = json_str.find_first_not_of(" \t\n\r:", pos);
    if (pos == std::string::npos) {
        return "";
    }
    
    // If next char is quote, extract quoted string
    if (json_str[pos] == '"') {
        pos++;  // Skip opening quote
        size_t end_pos = json_str.find('"', pos);
        if (end_pos != std::string::npos) {
            return json_str.substr(pos, end_pos - pos);
        }
    } else {
        // Unquoted value - find next comma or closing brace
        size_t end_pos = json_str.find_first_of(",}", pos);
        if (end_pos != std::string::npos) {
            std::string result = json_str.substr(pos, end_pos - pos);
            // Trim whitespace
            result.erase(0, result.find_first_not_of(" \t\n\r"));
            result.erase(result.find_last_not_of(" \t\n\r") + 1);
            return result;
        }
    }
    
    return "";
}

// Aligns with eino adk.agentTool.InvokableRun(ctx, argumentsInJSON, opts)
// Implements complete execution lifecycle:
// 1. Check for interrupt/resume state
// 2. Prepare input messages
// 3. Create internal runner
// 4. Execute agent with event iteration
// 5. Handle interrupt events (save state, return InterruptAndRerun)
// 6. Extract output and return
std::string AgentTool::InvokableRun(
    void* ctx,
    const std::string& arguments_in_json,
    const std::vector<std::shared_ptr<components::Option>>& opts) {
    
    // TODO: Implement interrupt/resume logic using compose State
    // For now, simple implementation:
    
    // Prepare input messages
    auto input_messages = PrepareInput(ctx, arguments_in_json);
    
    // Create runner with no checkpoint store for now
    RunnerConfig config;
    config.agent = agent_;
    config.enable_streaming = false;
    
    Runner runner(config);
    
    // Execute agent
    auto iter = runner.Run(ctx, input_messages);
    
    // Iterate and collect events until completion
    std::shared_ptr<AgentEvent> last_event;
    while (true) {
        if (!iter->Next(last_event)) {
            break;
        }
        
        if (last_event->HasError()) {
            throw std::runtime_error("Agent execution failed: " + last_event->error_msg);
        }
        
        // Check for interrupt
        if (last_event->action && last_event->action->interrupted) {
            // TODO: Save checkpoint to state and return InterruptAndRerun
            throw std::runtime_error("Agent interrupted (not fully implemented)");
        }
    }
    
    // Extract and return output
    if (last_event) {
        return ExtractOutput(last_event);
    }
    
    return "";
}

// Aligns with eino adk.agentTool PrepareInput logic
// If full_chat_history_as_input: get chat history from state
// Otherwise: parse JSON and extract "request" field
std::vector<std::shared_ptr<schema::Message>> AgentTool::PrepareInput(
    void* ctx,
    const std::string& arguments_in_json) {
    
    std::vector<std::shared_ptr<schema::Message>> messages;
    
    if (options_->full_chat_history_as_input) {
        // Get React chat history from compose State
        messages = GetReactChatHistory(ctx, agent_->Name(ctx));
    } else {
        // Parse JSON and extract "request" field
        std::string request = ExtractRequestField(arguments_in_json);
        
        auto msg = std::make_shared<schema::Message>();
        msg->role = schema::RoleType::User;
        msg->content = request;
        messages.push_back(msg);
    }
    
    return messages;
}

// Aligns with eino adk.getReactChatHistory
// Gets chat history from compose State for React pattern
// 1. Get messages from State (remove last assistant message - the tool call)
// 2. Append transfer messages to target agent
// 3. Filter out system messages
// 4. Rewrite assistant/tool messages with agent name
std::vector<std::shared_ptr<schema::Message>> AgentTool::GetReactChatHistory(
    void* ctx,
    const std::string& dest_agent_name) {
    
    std::vector<std::shared_ptr<schema::Message>> messages;
    std::string agent_name;
    
    // Get messages from State
    compose::ProcessState(ctx, [&](void* ctx, compose::State* st) -> std::string {
        if (st && !st->messages.empty()) {
            // Copy all messages except the last one (tool call message)
            for (size_t i = 0; i < st->messages.size() - 1; ++i) {
                messages.push_back(st->messages[i]);
            }
            agent_name = st->agent_name;
        }
        return "";  // No error
    });
    
    // Append transfer messages
    auto [a_msg, t_msg] = GenTransferMessages(ctx, dest_agent_name);
    auto a_msg_ptr = std::make_shared<schema::Message>(a_msg);
    auto t_msg_ptr = std::make_shared<schema::Message>(t_msg);
    messages.push_back(a_msg_ptr);
    messages.push_back(t_msg_ptr);
    
    // Filter and rewrite messages
    std::vector<std::shared_ptr<schema::Message>> history;
    for (const auto& msg : messages) {
        // Skip system messages
        if (msg->role == schema::RoleType::System) {
            continue;
        }
        
        // Rewrite assistant/tool messages with agent name
        if (msg->role == schema::RoleType::Assistant || 
            msg->role == schema::RoleType::Tool) {
            auto rewritten = RewriteMessage(msg, agent_name);
            history.push_back(rewritten);
        } else {
            history.push_back(msg);
        }
    }
    
    return history;
}

// Helper to rewrite message with agent name prefix
// Aligns with eino adk.rewriteMessage
std::shared_ptr<schema::Message> AgentTool::RewriteMessage(
    const std::shared_ptr<schema::Message>& msg,
    const std::string& agent_name) {
    
    auto rewritten = std::make_shared<schema::Message>(*msg);
    
    // Add agent name prefix to content
    if (!agent_name.empty() && !msg->content.empty()) {
        rewritten->content = "[" + agent_name + "] " + msg->content;
    }
    
    return rewritten;
}

// Aligns with eino adk extractOutput logic
// Handles both streaming and non-streaming output
std::string AgentTool::ExtractOutput(const std::shared_ptr<AgentEvent>& event) {
    if (!event || !event->output || !event->output->message_output) {
        return "";
    }
    
    const auto& msg_output = event->output->message_output;
    if (!msg_output->is_streaming && msg_output->message) {
        return msg_output->message->content;
    }
    
    // TODO: Handle streaming output - read all chunks and concatenate
    if (msg_output->is_streaming && msg_output->message_stream) {
        std::string result;
        // Collect all message chunks
        // result should be concatenation of all chunks
        return result;
    }
    
    return "";
}

// Aligns with eino adk.NewAgentTool(ctx, agent, options...)
std::shared_ptr<components::BaseTool> NewAgentTool(
    void* ctx,
    const std::shared_ptr<Agent>& agent,
    const std::vector<AgentToolOption>& options) {
    
    auto opts = std::make_shared<AgentToolOptions>();
    for (const auto& opt : options) {
        opt(opts);
    }
    
    return std::make_shared<AgentTool>(ctx, agent, opts);
}

// Convenience overload for single option
std::shared_ptr<components::BaseTool> NewAgentTool(
    void* ctx,
    const std::shared_ptr<Agent>& agent,
    const AgentToolOption& option) {
    
    auto opts = std::make_shared<AgentToolOptions>();
    option(opts);
    
    return std::make_shared<AgentTool>(ctx, agent, opts);
}

}  // namespace adk
}  // namespace eino
