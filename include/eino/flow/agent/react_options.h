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

#ifndef EINO_CPP_FLOW_AGENT_REACT_OPTIONS_H_
#define EINO_CPP_FLOW_AGENT_REACT_OPTIONS_H_

#include "../../compose/graph.h"
#include "../../components/model.h"
#include "../../components/tool.h"
#include "../../schema/types.h"
#include "../../callbacks/callbacks.h"
#include <functional>
#include <memory>
#include <vector>

namespace eino {
namespace flow {
namespace agent {
namespace react {

// AgentOption type for configuring React agent
// Aligns with: eino/flow/agent/agent_option.go:AgentOption
using AgentOption = compose::Option;

// WithToolOptions returns an agent option that specifies tool.Option for the tools in agent
// Aligns with: eino/flow/agent/react/option.go:WithToolOptions
inline AgentOption WithToolOptions(const std::vector<components::tool::Option>& opts) {
    return compose::WithToolsNodeOption(compose::WithToolOption(opts));
}

// WithChatModelOptions returns an agent option that specifies model.Option for the chat model in agent
// Aligns with: eino/flow/agent/react/option.go:WithChatModelOptions
inline AgentOption WithChatModelOptions(const std::vector<components::model::Option>& opts) {
    return compose::WithChatModelOption(opts);
}

// WithToolList returns an agent option that specifies the list of tools
// Deprecated: Use WithTools instead to configure both ChatModel and ToolsNode
// Aligns with: eino/flow/agent/react/option.go:WithToolList
inline AgentOption WithToolList(const std::vector<std::shared_ptr<components::tool::BaseTool>>& tools) {
    return compose::WithToolsNodeOption(compose::WithToolList(tools));
}

// WithTools configures a React agent with a list of tools
// It performs two essential operations:
//  1. Extracts tool information for the chat model to understand available tools
//  2. Registers the actual tool implementations for execution
//
// Parameters:
//   - ctx: The context for the operation
//   - tools: A variadic list of tools
//
// Returns:
//   - vector<AgentOption>: A vector containing exactly 2 agent options:
//     - Option 1: Configures the chat model with tool schemas
//     - Option 2: Registers the tool implementations
//   - string: Error message if any tool's Info() method fails
//
// Aligns with: eino/flow/agent/react/option.go:WithTools
inline std::pair<std::vector<AgentOption>, std::string> WithTools(
    std::shared_ptr<compose::Context> ctx,
    const std::vector<std::shared_ptr<components::tool::BaseTool>>& tools) {
    
    std::vector<schema::ToolInfo> tool_infos;
    tool_infos.reserve(tools.size());
    
    for (const auto& tool : tools) {
        try {
            auto info = tool->Info(ctx);
            tool_infos.push_back(info);
        } catch (const std::exception& e) {
            return {{}, std::string("failed to get tool info: ") + e.what()};
        }
    }
    
    std::vector<AgentOption> opts(2);
    opts[0] = compose::WithChatModelOption(components::model::WithTools(tool_infos));
    opts[1] = compose::WithToolsNodeOption(compose::WithToolList(tools));
    
    return {opts, ""};
}

// MessageFuture interface for retrieving messages asynchronously
// Aligns with: eino/flow/agent/react/option.go:MessageFuture
class MessageFuture {
public:
    virtual ~MessageFuture() = default;
    
    // Iterator for messages
    class MessageIterator {
    public:
        virtual ~MessageIterator() = default;
        virtual bool Next(schema::Message& msg, std::string& error) = 0;
    };
    
    // Iterator for message streams
    class MessageStreamIterator {
    public:
        virtual ~MessageStreamIterator() = default;
        virtual bool Next(std::shared_ptr<schema::StreamReader<schema::Message>>& stream, 
                         std::string& error) = 0;
    };
    
    // GetMessages returns an iterator for retrieving messages generated during "agent.Generate" calls
    virtual std::shared_ptr<MessageIterator> GetMessages() = 0;
    
    // GetMessageStreams returns an iterator for retrieving streaming messages generated during "agent.Stream" calls
    virtual std::shared_ptr<MessageStreamIterator> GetMessageStreams() = 0;
};

// WithMessageFuture returns an agent option and a MessageFuture interface instance
// The option configures the agent to collect messages generated during execution,
// while the MessageFuture interface allows users to asynchronously retrieve these messages.
//
// Aligns with: eino/flow/agent/react/option.go:WithMessageFuture
inline std::pair<AgentOption, std::shared_ptr<MessageFuture>> WithMessageFuture() {
    // Implementation requires callback handler infrastructure
    // This is a placeholder for the interface
    
    class MessageFutureImpl : public MessageFuture {
    public:
        class MessageIteratorImpl : public MessageIterator {
        public:
            bool Next(schema::Message& msg, std::string& error) override {
                // Implementation with unbounded channel
                return false;
            }
        };
        
        class MessageStreamIteratorImpl : public MessageStreamIterator {
        public:
            bool Next(std::shared_ptr<schema::StreamReader<schema::Message>>& stream,
                     std::string& error) override {
                // Implementation with unbounded channel
                return false;
            }
        };
        
        std::shared_ptr<MessageIterator> GetMessages() override {
            return std::make_shared<MessageIteratorImpl>();
        }
        
        std::shared_ptr<MessageStreamIterator> GetMessageStreams() override {
            return std::make_shared<MessageStreamIteratorImpl>();
        }
    };
    
    auto future = std::make_shared<MessageFutureImpl>();
    
    // Create callback option that populates the future
    AgentOption option = compose::WithCallbacks(/* callback handler */);
    
    return {option, future};
}

// BuildAgentCallback builds a callback handler for agent
// Aligns with: eino/flow/agent/react/callback.go:BuildAgentCallback
inline std::shared_ptr<callbacks::Handler> BuildAgentCallback(
    std::shared_ptr<callbacks::ModelCallbackHandler> model_handler,
    std::shared_ptr<callbacks::ToolCallbackHandler> tool_handler) {
    
    return callbacks::NewHandlerHelper()
        .ChatModel(model_handler)
        .Tool(tool_handler)
        .Build();
}

} // namespace react
} // namespace agent
} // namespace flow
} // namespace eino

#endif // EINO_CPP_FLOW_AGENT_REACT_OPTIONS_H_
