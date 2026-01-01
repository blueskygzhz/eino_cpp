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

#ifndef EINO_CPP_ADK_PREBUILT_REACT_H_
#define EINO_CPP_ADK_PREBUILT_REACT_H_

#include "../agent.h"
#include "../chat_model_agent.h"
#include "../../components/component.h"
#include <memory>
#include <vector>
#include <string>
#include <map>
#include <functional>

namespace eino {
namespace adk {
namespace prebuilt {

// MessageModifier modifies input messages before the model is called
// Useful for adding system prompts or other messages
typedef std::function<std::vector<Message>(void* ctx, const std::vector<Message>& input)>
    MessageModifier;

// ReActConfig configuration for ReAct agent
// Aligns with eino flow/agent/react AgentConfig
struct ReActConfig {
    // ToolCallingChatModel is the chat model for handling user messages with tool calling
    void* chat_model = nullptr;  // ChatModel pointer
    
    // ToolsConfig configuration for tools node
    std::vector<void*> tools;  // BaseTool pointers
    
    // MessageModifier modifies input messages before the model is called
    // Useful for adding system prompt or other messages
    MessageModifier message_modifier = nullptr;
    
    // MessageRewriter modifies messages in state before the ChatModel is called
    // Takes accumulated messages in state, modifies them, and puts back into state
    // Useful for compressing message history or making cross-model-call changes
    MessageModifier message_rewriter = nullptr;
    
    // MaxStep maximum steps in the ReAct loop
    // Default: 12 steps (node count + 10)
    int max_step = 12;
    
    // Tools that will make agent return directly when called
    // When multiple tools are called and more than one is in return_directly list,
    // only the first one will be returned
    std::map<std::string, bool> tools_return_directly;
    
    // StreamToolCallChecker function to determine if streaming output contains tool calls
    // Different models output tool calls in streaming mode differently:
    // - Some models (like OpenAI) output tool calls directly
    // - Others (like Claude) output text first, then tool calls
    // Optional: By default checks if first chunk contains tool calls
    typedef std::function<bool(void* ctx, const std::vector<Message>& stream_output)>
        StreamToolCallChecker;
    StreamToolCallChecker stream_tool_call_checker = nullptr;
    
    // Optional node names for customization
    std::string graph_name = "ReActAgent";
    std::string model_node_name = "ChatModel";
    std::string tools_node_name = "Tools";
};

// ReActAgent implements the ReAct (Reasoning + Acting) agent pattern
// ReAct agent handles user messages with a chat model and tools
// - Calls the chat model
// - If message contains tool calls, it calls the tools
// - If tool is configured to return directly, returns directly
// - Otherwise, continues calling the chat model until no tool calls
class ReActAgent : public ResumableAgent {
public:
    explicit ReActAgent(void* ctx, const std::shared_ptr<ReActConfig>& config);
    ~ReActAgent() override = default;

    std::string Name(void* ctx) override;
    std::string Description(void* ctx) override;

    std::shared_ptr<AsyncIterator<std::shared_ptr<AgentEvent>>> Run(
        void* ctx,
        const std::shared_ptr<AgentInput>& input,
        const std::vector<std::shared_ptr<AgentRunOption>>& options) override;

    std::shared_ptr<AsyncIterator<std::shared_ptr<AgentEvent>>> Resume(
        void* ctx,
        const std::shared_ptr<ResumeInfo>& info,
        const std::vector<std::shared_ptr<AgentRunOption>>& options) override;

private:
    std::string name_;
    std::string description_;
    std::shared_ptr<ReActConfig> config_;
    std::shared_ptr<Agent> underlying_agent_;
};

// NewReActAgent creates a ReAct agent that combines reasoning and tool use
// The agent follows a Thought-Action-Observation loop
// Aligns with eino flow/agent/react NewAgent implementation
std::shared_ptr<ReActAgent> NewReActAgent(
    void* ctx,
    const std::shared_ptr<ReActConfig>& config);

}  // namespace prebuilt
}  // namespace adk
}  // namespace eino

#endif  // EINO_CPP_ADK_PREBUILT_REACT_H_
