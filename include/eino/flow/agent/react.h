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

#ifndef EINO_CPP_FLOW_AGENT_REACT_H_
#define EINO_CPP_FLOW_AGENT_REACT_H_

#include "../../compose/graph.h"
#include "../../components/model.h"
#include "../../components/tool.h"
#include "../../schema/types.h"
#include "../../schema/stream.h"
#include "react_state.h"
#include "react_options.h"
#include <functional>
#include <memory>
#include <vector>
#include <map>
#include <set>

namespace eino {
namespace flow {
namespace agent {

// MessageModifier modifies input messages before the model is called
// Aligns with: eino/flow/agent/react/react.go:MessageModifier
using MessageModifier = std::function<std::vector<schema::Message>(
    std::shared_ptr<compose::Context>, const std::vector<schema::Message>&)>;

// StreamToolCallChecker checks if model output contains tool calls
// Returns true if tool calls are found, false otherwise
// The checker MUST close the stream before returning
// Aligns with: eino/flow/agent/react/react.go:StreamToolCallChecker
using StreamToolCallChecker = std::function<bool(
    std::shared_ptr<compose::Context>, 
    std::shared_ptr<schema::StreamReader<schema::Message>>,
    std::string& error)>;

// ReActConfig is the configuration for ReAct agent
// Aligns with: eino/flow/agent/react/react.go:AgentConfig
struct ReActConfig {
    // ToolCallingModel is the chat model with tool calling capability (recommended)
    // This is the recommended model field to use.
    std::shared_ptr<components::ChatModel> tool_calling_model;
    
    // Deprecated: Use tool_calling_model instead
    std::shared_ptr<components::ChatModel> model;
    
    // Tools configuration
    compose::ToolsNodeConfig tools_config;
    
    // MessageModifier
    // Modify the input messages before the model is called, it's useful when you want 
    // to add some system prompt or other messages.
    MessageModifier message_modifier;
    
    // MessageRewriter modifies message in the state, before the ChatModel is called.
    // It takes the messages stored accumulated in state, modify them, and put the 
    // modified version back into state.
    // Useful for compressing message history to fit the model context window,
    // or if you want to make changes to messages that take effect across multiple model calls.
    // NOTE: if both MessageModifier and MessageRewriter are set, MessageRewriter will 
    // be called before MessageModifier.
    MessageModifier message_rewriter;
    
    // MaxStep
    // Default 12 of steps in pregel (node num + 10).
    int max_step = 12;
    
    // Tools that will make agent return directly when the tool is called.
    // When multiple tools are called and more than one tool is in the return directly list,
    // only the first one will be returned.
    std::set<std::string> tool_return_directly;
    
    // StreamToolCallChecker is a function to determine whether the model's streaming 
    // output contains tool calls.
    // Different models have different ways of outputting tool calls in streaming mode:
    // - Some models (like OpenAI) output tool calls directly
    // - Others (like Claude) output text first, then tool calls
    // This handler allows custom logic to check for tool calls in the stream.
    // It should return:
    // - true if the output contains tool calls and agent should continue processing
    // - false if no tool calls and agent should stop
    // Note: This field only needs to be configured when using streaming mode
    // Note: The handler MUST close the modelOutput stream before returning
    // Optional. By default, it checks if the first chunk contains tool calls.
    // Note: The default implementation does not work well with Claude, which typically 
    // outputs tool calls after text content.
    // Note: If your ChatModel doesn't output tool calls first, you can try adding prompts 
    // to constrain the model from generating extra text during the tool call.
    StreamToolCallChecker stream_tool_call_checker;
    
    // GraphName is the graph name of the ReAct Agent.
    // Optional. Default `ReActAgent`.
    std::string graph_name = "ReActAgent";
    
    // ModelNodeName is the node name of the model node in the ReAct Agent graph.
    // Optional. Default `ChatModel`.
    std::string model_node_name = "ChatModel";
    
    // ToolsNodeName is the node name of the tools node in the ReAct Agent graph.
    // Optional. Default `Tools`.
    std::string tools_node_name = "Tools";
};

// ReActAgent implements the ReAct (Reasoning + Acting) agent pattern
// ReAct agent is a simple agent that handles user messages with a chat model and tools.
// ReAct will call the chat model, if the message contains tool calls, it will call the tools.
// if the tool is configured to return directly, ReAct will return directly.
// otherwise, ReAct will continue to call the chat model until the message contains no tool calls.
//
// Aligns with: eino/flow/agent/react/react.go:Agent
class ReActAgent {
public:
    virtual ~ReActAgent() = default;
    
    // Create a new ReActAgent with configuration
    // IMPORTANT!! For models that don't output tool calls in the first streaming chunk 
    // (e.g. Claude) the default StreamToolCallChecker may not work properly since it only 
    // checks the first chunk for tool calls. In such cases, you need to implement a custom 
    // StreamToolCallChecker that can properly detect tool calls.
    //
    // Aligns with: eino/flow/agent/react/react.go:NewAgent
    static std::shared_ptr<ReActAgent> Create(
        std::shared_ptr<compose::Context> ctx,
        const ReActConfig& config);
    
    // Generate generates a response from the agent
    // Aligns with: eino/flow/agent/react/react.go:Agent.Generate
    virtual schema::Message Generate(
        std::shared_ptr<compose::Context> ctx,
        const std::vector<schema::Message>& messages,
        const std::vector<react::AgentOption>& opts = {}) = 0;
    
    // Stream calls the agent and returns a stream response
    // Aligns with: eino/flow/agent/react/react.go:Agent.Stream
    virtual std::shared_ptr<schema::StreamReader<schema::Message>> Stream(
        std::shared_ptr<compose::Context> ctx,
        const std::vector<schema::Message>& messages,
        const std::vector<react::AgentOption>& opts = {}) = 0;
    
    // ExportGraph exports the underlying graph from Agent, along with the GraphAddNodeOpt 
    // to be used when adding this graph to another graph.
    // Aligns with: eino/flow/agent/react/react.go:Agent.ExportGraph
    virtual std::pair<std::shared_ptr<compose::Graph>, std::vector<compose::GraphAddNodeOpt>> 
        ExportGraph() = 0;
};

// Helper function to create persona modifier
// Deprecated: This approach of adding persona involves unnecessary slice copying overhead.
// Instead, directly include the persona message in the input messages when calling Generate or Stream.
//
// Aligns with: eino/flow/agent/react/react.go:NewPersonaModifier
inline MessageModifier NewPersonaModifier(const std::string& persona) {
    return [persona](std::shared_ptr<compose::Context> ctx, 
                     const std::vector<schema::Message>& input) {
        std::vector<schema::Message> result;
        result.reserve(input.size() + 1);
        
        // Add system message with persona
        schema::Message system_msg;
        system_msg.role = schema::RoleType::kSystem;
        system_msg.content = persona;
        result.push_back(system_msg);
        
        // Add input messages
        result.insert(result.end(), input.begin(), input.end());
        
        return result;
    };
}

// Constants for node keys
// Aligns with: eino/flow/agent/react/react.go:const
constexpr const char* kGraphName = "ReActAgent";
constexpr const char* kModelNodeName = "ChatModel";
constexpr const char* kToolsNodeName = "Tools";

} // namespace agent
} // namespace flow
} // namespace eino

#endif // EINO_CPP_FLOW_AGENT_REACT_H_
