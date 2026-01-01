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

#ifndef EINO_CPP_FLOW_AGENT_REACT_STATE_H_
#define EINO_CPP_FLOW_AGENT_REACT_STATE_H_

#include "../../schema/types.h"
#include "../../compose/context.h"
#include "../../compose/state.h"
#include <vector>
#include <string>
#include <memory>

namespace eino {
namespace flow {
namespace agent {
namespace react {

// ReActState holds the internal state of a ReAct agent execution
// Aligns with: eino/flow/agent/react/react.go:state
struct ReActState {
    // Messages accumulated during agent execution
    std::vector<schema::Message> messages;
    
    // Tool call ID that should return directly
    // When set, the agent will return the result of this tool call immediately
    // without further processing
    std::string return_directly_tool_call_id;
    
    ReActState() = default;
    
    // Serialization support for checkpoint/resume
    // Aligns with: eino/flow/agent/react/react.go:init (RegisterName)
    static const char* GetTypeName() {
        return "_eino_react_state";
    }
};

// SetReturnDirectly is a helper function that can be called within a tool's execution
// It signals the ReAct agent to stop further processing and return the result of 
// the current tool call directly.
//
// This is useful when the tool's output is the final answer and no more steps are needed.
// Note: If multiple tools call this function in the same step, only the last call will take effect.
// This setting has a higher priority than the AgentConfig.ToolReturnDirectly.
//
// Aligns with: eino/flow/agent/react/react.go:SetReturnDirectly
inline std::string SetReturnDirectly(std::shared_ptr<compose::Context> ctx) {
    if (!ctx) {
        return "context is null";
    }
    
    try {
        // Get current tool call ID from context
        std::string tool_call_id = compose::GetToolCallID(ctx);
        
        if (tool_call_id.empty()) {
            return "tool call ID not found in context";
        }
        
        // Update state
        auto err = compose::ProcessState<ReActState>(ctx, 
            [tool_call_id](std::shared_ptr<compose::Context> ctx, 
                          std::shared_ptr<ReActState> state) -> std::string {
                if (!state) {
                    return "state is null";
                }
                state->return_directly_tool_call_id = tool_call_id;
                return "";
            });
        
        return err;
        
    } catch (const std::exception& e) {
        return std::string("failed to set return directly: ") + e.what();
    }
}

// GetReturnDirectlyToolCallID gets the return directly tool call ID from input message
// based on the configured tool return directly set
//
// Aligns with: eino/flow/agent/react/react.go:getReturnDirectlyToolCallID
inline std::string GetReturnDirectlyToolCallID(
    const schema::Message& input,
    const std::set<std::string>& tool_return_directly) {
    
    if (tool_return_directly.empty()) {
        return "";
    }
    
    for (const auto& tool_call : input.tool_calls) {
        if (tool_return_directly.count(tool_call.function.name) > 0) {
            return tool_call.id;
        }
    }
    
    return "";
}

// firstChunkStreamToolCallChecker is the default implementation for checking
// if the model's streaming output contains tool calls
//
// It checks the first non-empty chunk for tool calls:
// - Returns true if tool calls are found
// - Returns false if text content is found first
// - Skips empty chunks
//
// IMPORTANT: This implementation does NOT work well with models that output
// text before tool calls (e.g., Claude). For such models, you need to provide
// a custom StreamToolCallChecker in AgentConfig.
//
// Aligns with: eino/flow/agent/react/react.go:firstChunkStreamToolCallChecker
inline bool FirstChunkStreamToolCallChecker(
    std::shared_ptr<compose::Context> ctx,
    std::shared_ptr<schema::StreamReader<schema::Message>> sr,
    std::string& error) {
    
    if (!sr) {
        error = "stream reader is null";
        return false;
    }
    
    // Ensure stream is closed when done
    struct StreamCloser {
        std::shared_ptr<schema::StreamReader<schema::Message>> stream;
        ~StreamCloser() { if (stream) stream->Close(); }
    } closer{sr};
    
    while (true) {
        schema::Message msg;
        std::string recv_error;
        
        bool has_data = sr->Recv(msg, recv_error);
        
        if (!has_data) {
            // EOF - no tool calls found
            if (recv_error == "EOF") {
                error = "";
                return false;
            }
            // Other error
            error = recv_error;
            return false;
        }
        
        // Error in message
        if (!recv_error.empty()) {
            error = recv_error;
            return false;
        }
        
        // Found tool calls
        if (!msg.tool_calls.empty()) {
            error = "";
            return true;
        }
        
        // Skip empty chunks at the front
        if (msg.content.empty()) {
            continue;
        }
        
        // Found text content before tool calls - no tool calls
        error = "";
        return false;
    }
}

} // namespace react
} // namespace agent
} // namespace flow
} // namespace eino

#endif // EINO_CPP_FLOW_AGENT_REACT_STATE_H_
