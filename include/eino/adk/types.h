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

#ifndef EINO_CPP_ADK_TYPES_H_
#define EINO_CPP_ADK_TYPES_H_

// Aligns with eino adk types
// Defines core data structures for agent execution and composition

#include "../schema/types.h"
#include <vector>
#include <map>
#include <string>
#include <memory>
#include <cstdint>
#include <memory>

namespace eino {
namespace adk {

// Aligns with eino adk.Message type
// Type alias for Message pointer
typedef schema::Message* Message;

// Aligns with eino adk.MessageStream type
// Type alias for stream reader of messages
typedef schema::StreamReader<Message>* MessageStream;

// Aligns with eino adk.MessageVariant struct
// Represents either a single message or a message stream
struct MessageVariant {
    // Whether the output is streaming or not
    bool is_streaming = false;
    
    // Single message (non-streaming)
    Message message = nullptr;
    
    // Message stream (streaming)
    MessageStream message_stream = nullptr;
    
    // Message role: Assistant or Tool
    schema::RoleType role;
    
    // Tool name (only used when role is Tool)
    std::string tool_name;
    
    // GetMessage returns the complete message, concatenating stream if necessary
    // Aligns with eino adk.MessageVariant.GetMessage()
    std::pair<Message, std::string> GetMessage() const;
};

// Aligns with eino adk.TransferToAgentAction struct
// Action to transfer execution to another agent
struct TransferToAgentAction {
    // Target agent name
    std::string dest_agent_name;
};

// Aligns with eino adk.AgentOutput struct
// Output from agent execution
struct AgentOutput {
    // Message output (either streaming or non-streaming)
    std::shared_ptr<MessageVariant> message_output;
    
    // Custom application-specific output
    std::shared_ptr<void> customized_output;
};

// Aligns with eino adk.InterruptInfo struct
// Information about an interrupt event
struct InterruptInfo {
    // Serialized interrupt data (type depends on source)
    std::shared_ptr<void> data;
};

// Aligns with eino adk.AgentAction struct
// Action taken by an agent after execution
struct AgentAction {
    // Exit signal: agent is complete
    bool exit = false;
    
    // Interrupt: agent interrupted, needs resume
    std::shared_ptr<InterruptInfo> interrupted;
    
    // Transfer to another agent
    std::shared_ptr<TransferToAgentAction> transfer_to_agent;
    
    // Break loop signal (for workflow agents)
    std::shared_ptr<void> break_loop;  // BreakLoopAction pointer
    
    // Custom application-specific action
    std::shared_ptr<void> customized_action;
};

// Aligns with eino adk.RunStep struct
// Represents one step in the execution path
struct RunStep {
    // Agent name for this step
    std::string agent_name;

    std::string String() const {
        return agent_name;
    }

    bool Equals(const RunStep& other) const {
        return agent_name == other.agent_name;
    }
};

// Aligns with eino adk.AgentEvent struct
// Event emitted during agent execution
struct AgentEvent {
    // Name of the agent that produced this event
    std::string agent_name;
    
    // Execution path (sequence of agents)
    std::vector<RunStep> run_path;
    
    // Output from agent execution
    std::shared_ptr<AgentOutput> output;
    
    // Action from agent execution
    std::shared_ptr<AgentAction> action;
    
    // Error if execution failed
    std::shared_ptr<std::exception> error;
    std::string error_msg;  // Error message string

    bool HasError() const {
        return error || !error_msg.empty();
    }
};

// Aligns with eino adk.AgentInput struct
// Input to agent execution
struct AgentInput {
    // Sequence of messages (chat history)
    std::vector<Message> messages;
    
    // Whether to enable streaming output
    bool enable_streaming = false;
};

// Aligns with eino adk.ResumeInfo struct
// Information for resuming from interrupt
struct ResumeInfo {
    // Enable streaming for resumed execution
    bool enable_streaming = false;
    
    // Interrupt information to resume from
    std::shared_ptr<InterruptInfo> interrupt_info;
};

// Aligns with eino adk.RunContext internal structure
// Execution context for run operations
struct RunContext {
    // Root input for the entire run
    std::shared_ptr<AgentInput> root_input;
    
    // Path of agents executed
    std::vector<RunStep> run_path;
    
    // Session-local values
    std::map<std::string, std::shared_ptr<void>> session_values;

    bool IsRoot() const {
        return run_path.size() == 1;
    }

    std::unique_ptr<RunContext> DeepCopy() const {
        auto copied = std::make_unique<RunContext>();
        copied->root_input = root_input;
        copied->run_path = run_path;
        copied->session_values = session_values;
        return copied;
    }
};

// Aligns with eino adk.WorkflowInterruptInfo struct
// Interrupt information for workflow agents
struct WorkflowInterruptInfo {
    // Original input to the workflow
    std::shared_ptr<AgentInput> orig_input;
    
    // For sequential: which sub-agent interrupted
    int sequential_interrupt_index = -1;
    std::shared_ptr<InterruptInfo> sequential_interrupt_info;
    
    // For loop: which iteration we're on
    int loop_iterations = 0;
    
    // For parallel: which sub-agents were interrupted (index -> interrupt info)
    std::map<int, std::shared_ptr<InterruptInfo>> parallel_interrupt_info;
};

// Aligns with eino compose.State struct
// State for ReAct and compose workflow execution
struct State {
    // All messages in the conversation
    std::vector<Message> messages;
    
    // Whether agent returned directly without more tool calls
    bool has_return_directly = false;
    std::string return_directly_tool_call_id;
    
    // Tool generation actions
    std::map<std::string, std::shared_ptr<AgentAction>> tool_gen_actions;
    
    // Current agent name
    std::string agent_name;
    
    // Interrupt data for agent tools (tool_call_id -> interrupt info)
    std::map<std::string, std::shared_ptr<void>> agent_tool_interrupt_data;
    
    // Remaining iterations for loop agents
    int32_t remaining_iterations = 0;
};

}  // namespace adk
}  // namespace eino

#endif  // EINO_CPP_ADK_TYPES_H_
