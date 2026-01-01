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

#include "eino/adk/interface.h"
#include "eino/adk/context.h"
#include "eino/adk/agent.h"
#include "eino/schema/types.h"
#include "eino/components/tool/tool.h"
#include <sstream>
#include <random>

namespace eino {
namespace adk {

// ============================================================================
// Session Management Functions
// ============================================================================

std::map<std::string, void*> GetSessionValues(void* ctx) {
    auto session = ContextManager::GetInstance().GetRunSession(ctx);
    if (session) {
        return session->GetValues();
    }
    return std::map<std::string, void*>();
}

void AddSessionValue(void* ctx, const std::string& key, void* value) {
    auto session = ContextManager::GetInstance().GetRunSession(ctx);
    if (session) {
        session->AddValue(key, value);
    }
}

void* GetSessionValue(void* ctx, const std::string& key, bool* found) {
    auto session = ContextManager::GetInstance().GetRunSession(ctx);
    if (session) {
        return session->GetValue(key, found);
    }
    if (found) *found = false;
    return nullptr;
}

void AddSessionValues(void* ctx, const std::map<std::string, void*>& values) {
    auto session = ContextManager::GetInstance().GetRunSession(ctx);
    if (session) {
        session->AddValues(values);
    }
}

void* ClearRunContext(void* ctx) {
    ContextManager::GetInstance().ClearContext(ctx);
    return nullptr;
}

// ============================================================================
// Tool Constants and Names
// ============================================================================

const std::string kTransferToAgentToolName = "transfer_to_agent";
const std::string kTransferToAgentToolDesc = "Transfer the question to another agent.";
const std::string kExitToolName = "exit";
const std::string kExitToolDesc = "Exit the agent process and return the final result.";

const std::string& GetTransferToAgentToolName() {
    return kTransferToAgentToolName;
}

const std::string& GetTransferToAgentToolDesc() {
    return kTransferToAgentToolDesc;
}

const std::string& GetExitToolName() {
    return kExitToolName;
}

const std::string& GetExitToolDesc() {
    return kExitToolDesc;
}

// ============================================================================
// Transfer Tool Output Helper
// ============================================================================

std::string TransferToAgentToolOutput(const std::string& dest_agent_name) {
    std::ostringstream oss;
    oss << "Transferred to agent: " << dest_agent_name;
    return oss.str();
}

// ============================================================================
// GenTransferMessages - Generate transfer action messages
// Aligns with eino/adk/utils.go::GenTransferMessages
// ============================================================================

std::pair<schema::Message, schema::Message> GenTransferMessages(
    void* ctx,
    const std::string& dest_agent_name) {
    
    // Generate tool call ID (UUID-like)
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    static const char* hex_chars = "0123456789abcdef";
    
    std::string tool_call_id;
    tool_call_id.reserve(36);
    for (int i = 0; i < 36; ++i) {
        if (i == 8 || i == 13 || i == 18 || i == 23) {
            tool_call_id += '-';
        } else {
            tool_call_id += hex_chars[dis(gen)];
        }
    }
    
    // Create assistant message with tool call
    schema::ToolCall tool_call;
    tool_call.id = tool_call_id;
    tool_call.function.name = kTransferToAgentToolName;
    tool_call.function.arguments = dest_agent_name;
    
    schema::Message assistant_msg = schema::AssistantMessage("", {tool_call});
    
    // Create tool message with transfer result
    schema::Message tool_msg = schema::ToolMessage(
        TransferToAgentToolOutput(dest_agent_name),
        tool_call_id);
    tool_msg->tool_name = kTransferToAgentToolName;
    
    return {assistant_msg, tool_msg};
}

// ============================================================================
// Exit Tool Factory
// ============================================================================

std::shared_ptr<tool::BaseTool> NewExitTool() {
    // Exit tool implementation
    class ExitTool : public tool::InvokableTool {
    public:
        std::shared_ptr<schema::ToolInfo> Info(void* ctx) override {
            auto info = std::make_shared<schema::ToolInfo>();
            info->name = kExitToolName;
            info->description = kExitToolDesc;
            return info;
        }
        
        std::string InvokableRun(void* ctx, const std::string& arguments_json) override {
            // Exit tool just returns a confirmation
            return "Exit confirmed.";
        }
    };
    
    return std::make_shared<ExitTool>();
}

// ============================================================================
// Transfer Action Factory
// ============================================================================

std::shared_ptr<TransferToAgentAction> NewTransferToAgentAction(
    const std::string& dest_agent_name) {
    auto action = std::make_shared<TransferToAgentAction>();
    action->dest_agent_name = dest_agent_name;
    return action;
}

std::shared_ptr<AgentAction> NewExitAction() {
    auto action = std::make_shared<AgentAction>();
    action->exit = true;
    return action;
}

// ============================================================================
// Agent Event Helpers
// ============================================================================

std::shared_ptr<AgentEvent> EventFromMessage(
    schema::Message msg,
    schema::MessageStream msg_stream,
    schema::RoleType role,
    const std::string& tool_name) {
    
    auto event = std::make_shared<AgentEvent>();
    event->output = std::make_shared<AgentOutput>();
    event->output->message_output = std::make_shared<MessageVariant>();
    
    auto msg_output = event->output->message_output;
    msg_output->is_streaming = (msg_stream != nullptr);
    msg_output->message = msg;
    msg_output->message_stream = msg_stream;
    msg_output->role = role;
    msg_output->tool_name = tool_name;
    
    return event;
}

schema::Message GetMessage(const std::shared_ptr<AgentEvent>& event) {
    if (!event || !event->output || !event->output->message_output) {
        return nullptr;
    }
    
    auto msg_output = event->output->message_output;
    if (msg_output->is_streaming) {
        // Concatenate streaming messages
        // TODO: Implement stream concatenation
        return nullptr;
    }
    
    return msg_output->message;
}

std::shared_ptr<AgentEvent> CopyAgentEvent(const std::shared_ptr<AgentEvent>& event) {
    if (!event) {
        return nullptr;
    }
    
    auto copied = std::make_shared<AgentEvent>();
    copied->agent_name = event->agent_name;
    copied->run_path = event->run_path;  // Deep copy
    copied->action = event->action;
    copied->error = event->error;
    copied->error_msg = event->error_msg;
    
    if (event->output) {
        copied->output = std::make_shared<AgentOutput>();
        copied->output->customized_output = event->output->customized_output;
        
        if (event->output->message_output) {
            copied->output->message_output = std::make_shared<MessageVariant>();
            auto src = event->output->message_output;
            auto dst = copied->output->message_output;
            
            dst->is_streaming = src->is_streaming;
            dst->role = src->role;
            dst->tool_name = src->tool_name;
            
            if (src->is_streaming) {
                // Copy stream (requires stream copy support)
                dst->message_stream = src->message_stream;
            } else {
                dst->message = src->message;
            }
        }
    }
    
    return copied;
}

// ============================================================================
// Error Iterator Helper
// ============================================================================

std::shared_ptr<AsyncIterator<std::shared_ptr<AgentEvent>>> GenErrorIterator(
    const std::string& error_msg) {
    
    auto pair = NewAsyncIteratorPair<std::shared_ptr<AgentEvent>>();
    auto error_event = std::make_shared<AgentEvent>();
    error_event->error_msg = error_msg;
    pair.second->Send(error_event);
    pair.second->Close();
    return pair.first;
}

std::shared_ptr<AsyncIterator<std::shared_ptr<AgentEvent>>> GenErrorIterator(
    std::shared_ptr<std::exception> error) {
    
    auto pair = NewAsyncIteratorPair<std::shared_ptr<AgentEvent>>();
    auto error_event = std::make_shared<AgentEvent>();
    error_event->error = error;
    if (error) {
        error_event->error_msg = error->what();
    }
    pair.second->Send(error_event);
    pair.second->Close();
    return pair.first;
}

// ============================================================================
// Instruction Concatenation Helper
// ============================================================================

std::string ConcatInstructions(const std::vector<std::string>& instructions) {
    if (instructions.empty()) {
        return "";
    }
    
    std::ostringstream oss;
    oss << instructions[0];
    
    for (size_t i = 1; i < instructions.size(); ++i) {
        oss << "\n\n" << instructions[i];
    }
    
    return oss.str();
}

}  // namespace adk
}  // namespace eino
