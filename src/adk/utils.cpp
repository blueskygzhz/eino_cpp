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

#include "eino/adk/utils.h"
#include "eino/adk/flow_agent.h"
#include "eino/adk/agent.h"
#include "eino/schema/types.h"
#include <sstream>
#include <algorithm>

namespace eino {
namespace adk {

// ============================================================================
// Agent Configuration Options
// ============================================================================

std::shared_ptr<AgentRunOption> WithSessionValues(const std::map<std::string, void*>& values) {
    auto opt = std::make_shared<AgentRunOption>();
    opt->session_values = values;
    return opt;
}

std::shared_ptr<AgentRunOption> WithSkipTransferMessages() {
    auto opt = std::make_shared<AgentRunOption>();
    opt->skip_transfer_messages = true;
    return opt;
}

std::shared_ptr<AgentRunOption> WithCheckPointID(const std::string& id) {
    auto opt = std::make_shared<AgentRunOption>();
    opt->checkpoint_id = id;
    return opt;
}

CommonOptions GetCommonOptions(const std::vector<std::shared_ptr<AgentRunOption>>& opts) {
    CommonOptions common;
    for (const auto& opt : opts) {
        if (opt) {
            common.session_values.insert(opt->session_values.begin(), opt->session_values.end());
            if (!opt->checkpoint_id.empty()) {
                common.checkpoint_id = opt->checkpoint_id;
            }
            if (opt->skip_transfer_messages) {
                common.skip_transfer_messages = true;
            }
        }
    }
    return common;
}

// ============================================================================
// Message Utilities
// ============================================================================

std::shared_ptr<AgentEvent> CopyAgentEvent(const std::shared_ptr<AgentEvent>& ae) {
    if (!ae) {
        return nullptr;
    }
    
    auto copied = std::make_shared<AgentEvent>();
    copied->error_msg = ae->error_msg;
    
    // Copy action
    if (ae->action) {
        copied->action = std::make_shared<AgentAction>();
        copied->action->interrupted = ae->action->interrupted;
        copied->action->break_loop = ae->action->break_loop;
        
        if (ae->action->transfer_to_agent) {
            copied->action->transfer_to_agent = std::make_shared<TransferToAgentAction>();
            copied->action->transfer_to_agent->dest_agent_name = 
                ae->action->transfer_to_agent->dest_agent_name;
        }
    }
    
    // Copy output - critical: handle streaming correctly
    if (ae->output) {
        copied->output = std::make_shared<AgentOutput>();
        
        if (ae->output->message_output) {
            const auto& mv = ae->output->message_output;
            auto copied_mv = std::make_shared<MessageOutput>();
            
            copied_mv->role = mv->role;
            copied_mv->tool_name = mv->tool_name;
            copied_mv->is_streaming = mv->is_streaming;
            copied_mv->finish_reason = mv->finish_reason;
            
            // Critical: Handle message stream properly
            if (mv->is_streaming && mv->message_stream) {
                // Create 2 independent copies to avoid stream reuse issues
                auto copies = schema::StreamReader<Message>::Copy(mv->message_stream, 2);
                // Keep one copy for original event, one for copied event
                const_cast<std::shared_ptr<MessageOutput>&>(mv)->message_stream = copies[0];
                copied_mv->message_stream = copies[1];
            } else if (mv->message) {
                // Non-streaming: deep copy the message
                copied_mv->message = std::make_shared<Message>(*mv->message);
            }
            
            copied->output->message_output = copied_mv;
        }
    }
    
    return copied;
}

std::pair<Message, Message> GenTransferMessages(void* ctx, const std::string& dest_agent_name) {
    // Assistant message with tool call for transfer
    Message assistant_msg;
    assistant_msg.role = schema::RoleType::Assistant;
    assistant_msg.content = "";
    
    // Create tool call for transfer
    schema::ToolCall tool_call;
    tool_call.id = "transfer_" + dest_agent_name;
    tool_call.type = "function";
    tool_call.function.name = "transfer_to_" + dest_agent_name;
    tool_call.function.arguments = "{\"agent\":\"" + dest_agent_name + "\"}";
    
    assistant_msg.tool_calls.push_back(tool_call);
    
    // Tool message with transfer confirmation
    Message tool_msg;
    tool_msg.role = schema::RoleType::Tool;
    tool_msg.tool_call_id = tool_call.id;
    tool_msg.tool_name = tool_call.function.name;
    tool_msg.content = "Transferred to " + dest_agent_name;
    
    return {assistant_msg, tool_msg};
}

std::pair<Message, std::string> GetMessageFromEvent(std::shared_ptr<AgentEvent> event) {
    Message msg;
    std::string error;
    
    if (!event) {
        error = "null event";
        return {msg, error};
    }
    
    if (event->HasError()) {
        error = event->error_msg;
        return {msg, error};
    }
    
    if (event->output && event->output->message_output) {
        const auto& msg_output = event->output->message_output;
        if (!msg_output->is_streaming && msg_output->message) {
            return {*msg_output->message, ""};
        }
        
        // Handle streaming: concatenate all chunks
        if (msg_output->is_streaming && msg_output->message_stream) {
            // Create 2 copies: one to preserve original, one to read
            auto copies = schema::StreamReader<Message>::Copy(msg_output->message_stream, 2);
            
            // Concatenate from the read copy
            auto result = schema::ConcatMessageStream(copies[1]);
            if (!result.second.empty()) {
                return {Message{}, result.second};
            }
            
            return {result.first, ""};
        }
    }
    
    return {msg, "no message in event"};
}

// SetAutomaticClose - Set automatic close on MessageStream
// Aligns with eino adk.setAutomaticClose (utils.go:83-90)
void SetAutomaticClose(std::shared_ptr<AgentEvent> event) {
    if (!event || !event->output || !event->output->message_output) {
        return;
    }
    
    auto msg_output = event->output->message_output;
    if (!msg_output->is_streaming || !msg_output->message_stream) {
        return;
    }
    
    // Set automatic close on stream
    msg_output->message_stream->SetAutomaticClose();
}

// GetMessageFromWrappedEvent - Extract Message from AgentEvent
// Aligns with eino adk.getMessageFromWrappedEvent (utils.go:93-135)
Message GetMessageFromWrappedEvent(const std::shared_ptr<AgentEvent>& event) {
    if (!event) {
        return nullptr;
    }
    
    if (event->HasError()) {
        return nullptr;
    }
    
    if (!event->output || !event->output->message_output) {
        return nullptr;
    }
    
    auto msg_output = event->output->message_output;
    
    if (msg_output->is_streaming) {
        // Streaming mode: need to concatenate
        if (!msg_output->message_stream) {
            return nullptr;
        }
        
        std::vector<Message> chunks;
        Message chunk;
        while (msg_output->message_stream->Read(chunk)) {
            if (chunk) {
                chunks.push_back(chunk);
            }
        }
        
        if (chunks.empty()) {
            return nullptr;
        }
        
        if (chunks.size() == 1) {
            return chunks[0];
        }
        
        // Use schema::ConcatMessages to merge all chunks
        return schema::ConcatMessages(chunks);
        
    } else {
        // Non-streaming mode: return single message
        return msg_output->message;
    }
}

// ============================================================================
// Deterministic Transfer Implementation
// ============================================================================

// Wrapper agent that adds deterministic transfers
class AgentWithDeterministicTransfer : public Agent {
public:
    AgentWithDeterministicTransfer(
        std::shared_ptr<Agent> agent,
        const std::vector<std::string>& to_agent_names)
        : agent_(agent), to_agent_names_(to_agent_names) {}
    
    std::string Name(void* ctx) override {
        return agent_->Name(ctx);
    }
    
    std::string Description(void* ctx) override {
        return agent_->Description(ctx);
    }
    
    std::shared_ptr<AsyncIterator<std::shared_ptr<AgentEvent>>> Run(
        void* ctx,
        const std::shared_ptr<AgentInput>& input,
        const std::vector<std::shared_ptr<AgentRunOption>>& options) override {
        
        // Execute underlying agent
        auto inner_iter = agent_->Run(ctx, input, options);
        
        // Create new iterator to append transfer messages
        auto pair = NewAsyncIteratorPair<std::shared_ptr<AgentEvent>>();
        auto iterator = pair.first;
        auto generator = pair.second;
        
        // Launch thread to forward events and append transfers
        std::thread([inner_iter, generator, to_agent_names = to_agent_names_, ctx]() {
            bool interrupted = false;
            
            // Forward all events from inner agent
            while (true) {
                std::shared_ptr<AgentEvent> event;
                if (!inner_iter->Next(event)) {
                    break;
                }
                
                generator->Send(event);
                
                // Track if agent was interrupted
                if (event && event->action && event->action->interrupted) {
                    interrupted = true;
                } else {
                    interrupted = false;
                }
            }
            
            // If not interrupted, append transfer actions
            if (!interrupted) {
                for (const auto& to_agent_name : to_agent_names) {
                    auto [a_msg, t_msg] = GenTransferMessages(ctx, to_agent_name);
                    
                    // Send assistant message event
                    auto a_event = std::make_shared<AgentEvent>();
                    a_event->output = std::make_shared<AgentOutput>();
                    a_event->output->message_output = std::make_shared<MessageOutput>();
                    a_event->output->message_output->is_streaming = false;
                    a_event->output->message_output->message = std::make_shared<Message>(a_msg);
                    a_event->output->message_output->role = schema::RoleType::Assistant;
                    generator->Send(a_event);
                    
                    // Send tool message event with transfer action
                    auto t_event = std::make_shared<AgentEvent>();
                    t_event->output = std::make_shared<AgentOutput>();
                    t_event->output->message_output = std::make_shared<MessageOutput>();
                    t_event->output->message_output->is_streaming = false;
                    t_event->output->message_output->message = std::make_shared<Message>(t_msg);
                    t_event->output->message_output->role = schema::RoleType::Tool;
                    t_event->output->message_output->tool_name = t_msg.tool_name;
                    
                    // Add transfer action
                    t_event->action = std::make_shared<AgentAction>();
                    t_event->action->transfer_to_agent = std::make_shared<TransferToAgentAction>();
                    t_event->action->transfer_to_agent->dest_agent_name = to_agent_name;
                    
                    generator->Send(t_event);
                }
            }
            
            generator->Close();
        }).detach();
        
        return iterator;
    }
    
private:
    std::shared_ptr<Agent> agent_;
    std::vector<std::string> to_agent_names_;
};

// Aligns with eino adk.AgentWithDeterministicTransferTo
std::shared_ptr<Agent> AgentWithDeterministicTransferTo(
    const DeterministicTransferConfig& config) {
    
    if (!config.agent) {
        return nullptr;
    }
    
    return std::make_shared<AgentWithDeterministicTransfer>(
        config.agent, config.to_agent_names);
}

// ============================================================================
// Agent Tree Utilities
// ============================================================================

std::pair<std::shared_ptr<Agent>, std::string> SetSubAgents(
    void* ctx,
    std::shared_ptr<Agent> parent_agent,
    const std::vector<std::shared_ptr<Agent>>& sub_agents) {
    
    if (!parent_agent) {
        return {nullptr, "parent agent is null"};
    }
    
    // Try to cast to OnSubAgents interface
    auto flow_agent = std::dynamic_pointer_cast<FlowAgent>(parent_agent);
    if (flow_agent) {
        flow_agent->SetSubAgents(ctx, sub_agents);
        return {parent_agent, ""};
    }
    
    // Agent doesn't support sub-agents
    return {nullptr, "agent does not support sub-agents"};
}

std::shared_ptr<Agent> GetAgent(
    std::shared_ptr<Agent> root_agent,
    void* ctx,
    const std::string& name) {
    
    if (!root_agent) {
        return nullptr;
    }
    
    if (root_agent->Name(ctx) == name) {
        return root_agent;
    }
    
    // Try to get sub-agents if supported
    auto flow_agent = std::dynamic_pointer_cast<FlowAgent>(root_agent);
    if (flow_agent) {
        auto sub_agents = flow_agent->GetSubAgents();
        for (const auto& sub : sub_agents) {
            auto found = GetAgent(sub, ctx, name);
            if (found) {
                return found;
            }
        }
    }
    
    return nullptr;
}

// ============================================================================
// Context and Session Utilities
// ============================================================================

void* ClearRunContext(void* ctx) {
    // Create a new context without execution state
    // In real implementation, would copy context but clear run-specific data
    return ctx;
}

std::string GenerateTransferToolOutput(const std::string& dest_agent_name) {
    std::ostringstream oss;
    oss << "Transferred control to agent: " << dest_agent_name;
    return oss.str();
}

// ============================================================================
// Agent Configuration
// ============================================================================

std::shared_ptr<Agent> ConfigureAgent(
    std::shared_ptr<Agent> agent,
    const std::string& name,
    const std::string& description,
    HistoryRewriter rewriter,
    bool disallow_transfer_to_parent) {
    
    // For now, return agent as-is
    // Full implementation would wrap agent with configuration
    return agent;
}

}  // namespace adk
}  // namespace eino
