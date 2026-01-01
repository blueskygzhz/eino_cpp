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

#include "../../../include/eino/adk/prebuilt/react.h"
#include "../../../include/eino/adk/chat_model_agent.h"
#include "../../../include/eino/adk/context.h"
#include "../../../include/eino/adk/flow_agent.h"
#include "../../../include/eino/schema/types.h"

namespace eino {
namespace adk {
namespace prebuilt {

namespace {

// Session key constants for ReAct
constexpr const char* kReActSessionKeyMessages = "eino.react.messages";
constexpr const char* kReActSessionKeyReturnDirectlyToolCallID = "eino.react.return_directly_tool_call_id";
constexpr const char* kReActSessionKeyRemainingIterations = "eino.react.remaining_iterations";

// State structure for ReAct graph
struct ReActState {
    std::vector<Message> messages;
    std::string return_directly_tool_call_id;
    int remaining_iterations = 0;
};

}  // namespace

ReActAgent::ReActAgent(void* ctx, const std::shared_ptr<ReActConfig>& config)
    : config_(config) {
    
    name_ = config->graph_name;
    description_ = "ReAct agent combining reasoning and tool use";
    
    // Create ChatModelAgent as the underlying agent
    auto cm_config = std::make_shared<ChatModelAgentConfig>();
    cm_config->name = name_;
    cm_config->description = description_;
    
    // Configure tools
    cm_config->tools_config.return_directly = config->tools_return_directly;
    
    // Convert shared_ptr tools to raw pointers for ToolsConfig
    for (const auto& tool : config->tools) {
        cm_config->tools_config.tools.push_back(tool.get());
    }
    
    cm_config->model = config->chat_model.get();
    cm_config->max_iterations = config->max_step;
    cm_config->output_key = "output";
    
    // Create the underlying ChatModelAgent
    underlying_agent_ = NewChatModelAgent(ctx, cm_config);
}

ReActAgent::~ReActAgent() = default;

std::string ReActAgent::Name(void* ctx) {
    if (underlying_agent_) {
        return underlying_agent_->Name(ctx);
    }
    return name_;
}

std::string ReActAgent::Description(void* ctx) {
    if (underlying_agent_) {
        return underlying_agent_->Description(ctx);
    }
    return description_;
}

std::shared_ptr<AsyncIterator<std::shared_ptr<AgentEvent>>> ReActAgent::Run(
    void* ctx,
    const std::shared_ptr<AgentInput>& input,
    const std::vector<std::shared_ptr<AgentRunOption>>& options) {
    
    if (!underlying_agent_) {
        auto pair = NewAsyncIteratorPair<std::shared_ptr<AgentEvent>>();
        auto event = std::make_shared<AgentEvent>();
        event->error_msg = "ReActAgent: underlying_agent is null";
        pair.second->Send(event);
        pair.second->Close();
        return pair.first;
    }
    
    // Forward to underlying ChatModelAgent
    return underlying_agent_->Run(ctx, input, options);
}

std::shared_ptr<AsyncIterator<std::shared_ptr<AgentEvent>>> ReActAgent::Resume(
    void* ctx,
    const std::shared_ptr<ResumeInfo>& info,
    const std::vector<std::shared_ptr<AgentRunOption>>& options) {
    
    if (!underlying_agent_) {
        auto pair = NewAsyncIteratorPair<std::shared_ptr<AgentEvent>>();
        auto event = std::make_shared<AgentEvent>();
        event->error_msg = "ReActAgent: underlying_agent is null";
        pair.second->Send(event);
        pair.second->Close();
        return pair.first;
    }
    
    // Try to resume if underlying agent supports it
    auto resumable = std::dynamic_pointer_cast<ResumableAgent>(underlying_agent_);
    if (resumable) {
        return resumable->Resume(ctx, info, options);
    }
    
    auto pair = NewAsyncIteratorPair<std::shared_ptr<AgentEvent>>();
    auto event = std::make_shared<AgentEvent>();
    event->error_msg = "ReActAgent: underlying_agent does not support resume";
    pair.second->Send(event);
    pair.second->Close();
    return pair.first;
}

std::shared_ptr<ReActAgent> NewReActAgent(
    void* ctx,
    const std::shared_ptr<ReActConfig>& config) {
    
    if (!config || !config->chat_model) {
        return nullptr;
    }
    
    return std::make_shared<ReActAgent>(ctx, config);
}

}  // namespace prebuilt
}  // namespace adk
}  // namespace eino
