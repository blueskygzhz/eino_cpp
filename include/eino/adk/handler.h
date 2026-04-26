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

#ifndef EINO_CPP_ADK_HANDLER_H_
#define EINO_CPP_ADK_HANDLER_H_

// ChatModelAgentMiddleware interface and related types.
// Aligned with Go: adk/handler.go
//
// This interface is specifically designed for ChatModelAgent and agents built
// on top of it (e.g., DeepAgent).

#include <any>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "eino/adk/types.h"
#include "eino/internal/core/address.h"
#include "eino/schema/types.h"

namespace eino {

// Forward declarations
namespace components {
namespace model {
class BaseChatModel;
}  // namespace model
namespace tool {
class BaseTool;
}  // namespace tool
}  // namespace components

namespace adk {

// InvokableToolCallEndpoint is the function signature for invoking a tool synchronously.
// Aligned with Go: adk.InvokableToolCallEndpoint
using InvokableToolCallEndpoint = std::function<
    std::pair<std::string, std::string>(  // (result, error)
        const internal::core::ExecutionContext& ctx,
        const std::string& arguments_in_json)>;

// StreamableToolCallEndpoint is the function signature for invoking a tool with streaming output.
// Aligned with Go: adk.StreamableToolCallEndpoint
using StreamableToolCallEndpoint = std::function<
    std::pair<std::shared_ptr<schema::StreamReader<std::string>>, std::string>(  // (stream, error)
        const internal::core::ExecutionContext& ctx,
        const std::string& arguments_in_json)>;

// ToolContext provides metadata about the tool being wrapped.
// Aligned with Go: adk.ToolContext
struct ToolContext {
    std::string name;     // Name of the tool being wrapped
    std::string call_id;  // Unique identifier for this specific tool call
};

// ModelRetryConfig holds retry configuration for model calls.
// Aligned with Go: adk.ModelRetryConfig
struct ModelRetryConfig {
    int max_retries = 0;
    std::function<bool(const std::string&)> is_retryable;
};

// ModelContext contains context information passed to WrapModel.
// Aligned with Go: adk.ModelContext
struct ModelContext {
    // Tools contains the current tool list configured for the agent.
    std::vector<std::shared_ptr<schema::ToolInfo>> tools;

    // ModelRetryConfig contains the retry configuration for the model.
    std::shared_ptr<ModelRetryConfig> model_retry_config;
};

// ChatModelAgentState provides access to the conversation state.
// Aligned with Go: adk.ChatModelAgentState
struct ChatModelAgentState {
    // Messages: the conversation history
    std::vector<schema::Message*> messages;
};

// ChatModelAgentContext contains runtime information passed to handlers before each ChatModelAgent run.
// Aligned with Go: adk.ChatModelAgentContext
struct ChatModelAgentContext {
    // Instruction is the current instruction for the Agent execution.
    std::string instruction;

    // Tools are the raw tools currently configured for the Agent execution.
    std::vector<std::shared_ptr<void>> tools;  // BaseTool pointers

    // ReturnDirectly is the set of tool names configured to cause the Agent to return directly.
    std::map<std::string, bool> return_directly;
};

// ChatModelAgentMiddleware defines the interface for customizing ChatModelAgent behavior.
// Aligned with Go: adk.ChatModelAgentMiddleware
//
// Why ChatModelAgentMiddleware instead of AgentMiddleware?
// - Interface type is open for extension (vs struct type which is closed)
// - Hook methods return (context, ..., error) for direct context propagation
// - Wrapper methods enable context propagation through the wrapped endpoint chain
// - Configuration is centralized in struct fields rather than scattered in closures
class ChatModelAgentMiddleware {
public:
    virtual ~ChatModelAgentMiddleware() = default;

    // BeforeAgent is called before each agent run, allowing modification of
    // the agent's instruction and tools configuration.
    // Aligned with Go: ChatModelAgentMiddleware.BeforeAgent()
    virtual std::tuple<internal::core::ExecutionContext, std::shared_ptr<ChatModelAgentContext>, std::string>
    BeforeAgent(const internal::core::ExecutionContext& ctx,
                std::shared_ptr<ChatModelAgentContext> run_ctx) = 0;

    // BeforeModelRewriteState is called before each model invocation.
    // Aligned with Go: ChatModelAgentMiddleware.BeforeModelRewriteState()
    virtual std::tuple<internal::core::ExecutionContext, std::shared_ptr<ChatModelAgentState>, std::string>
    BeforeModelRewriteState(const internal::core::ExecutionContext& ctx,
                            std::shared_ptr<ChatModelAgentState> state,
                            const ModelContext& mc) = 0;

    // AfterModelRewriteState is called after each model invocation.
    // Aligned with Go: ChatModelAgentMiddleware.AfterModelRewriteState()
    virtual std::tuple<internal::core::ExecutionContext, std::shared_ptr<ChatModelAgentState>, std::string>
    AfterModelRewriteState(const internal::core::ExecutionContext& ctx,
                           std::shared_ptr<ChatModelAgentState> state,
                           const ModelContext& mc) = 0;

    // WrapInvokableToolCall wraps a tool's synchronous execution with custom behavior.
    // Aligned with Go: ChatModelAgentMiddleware.WrapInvokableToolCall()
    virtual std::pair<InvokableToolCallEndpoint, std::string>
    WrapInvokableToolCall(const internal::core::ExecutionContext& ctx,
                          InvokableToolCallEndpoint endpoint,
                          const ToolContext& t_ctx) = 0;

    // WrapStreamableToolCall wraps a tool's streaming execution with custom behavior.
    // Aligned with Go: ChatModelAgentMiddleware.WrapStreamableToolCall()
    virtual std::pair<StreamableToolCallEndpoint, std::string>
    WrapStreamableToolCall(const internal::core::ExecutionContext& ctx,
                           StreamableToolCallEndpoint endpoint,
                           const ToolContext& t_ctx) = 0;

    // WrapModel wraps a chat model with custom behavior.
    // Aligned with Go: ChatModelAgentMiddleware.WrapModel()
    virtual std::pair<std::shared_ptr<void>, std::string>  // (BaseChatModel, error)
    WrapModel(const internal::core::ExecutionContext& ctx,
              std::shared_ptr<void> model,  // BaseChatModel
              const ModelContext& mc) = 0;
};

// BaseChatModelAgentMiddleware provides default no-op implementations for ChatModelAgentMiddleware.
// Embed this in custom handlers to only override the methods you need.
// Aligned with Go: adk.BaseChatModelAgentMiddleware
class BaseChatModelAgentMiddleware : public ChatModelAgentMiddleware {
public:
    ~BaseChatModelAgentMiddleware() override = default;

    std::tuple<internal::core::ExecutionContext, std::shared_ptr<ChatModelAgentContext>, std::string>
    BeforeAgent(const internal::core::ExecutionContext& ctx,
                std::shared_ptr<ChatModelAgentContext> run_ctx) override {
        return {ctx, run_ctx, ""};
    }

    std::tuple<internal::core::ExecutionContext, std::shared_ptr<ChatModelAgentState>, std::string>
    BeforeModelRewriteState(const internal::core::ExecutionContext& ctx,
                            std::shared_ptr<ChatModelAgentState> state,
                            const ModelContext& /*mc*/) override {
        return {ctx, state, ""};
    }

    std::tuple<internal::core::ExecutionContext, std::shared_ptr<ChatModelAgentState>, std::string>
    AfterModelRewriteState(const internal::core::ExecutionContext& ctx,
                           std::shared_ptr<ChatModelAgentState> state,
                           const ModelContext& /*mc*/) override {
        return {ctx, state, ""};
    }

    std::pair<InvokableToolCallEndpoint, std::string>
    WrapInvokableToolCall(const internal::core::ExecutionContext& /*ctx*/,
                          InvokableToolCallEndpoint endpoint,
                          const ToolContext& /*t_ctx*/) override {
        return {endpoint, ""};
    }

    std::pair<StreamableToolCallEndpoint, std::string>
    WrapStreamableToolCall(const internal::core::ExecutionContext& /*ctx*/,
                           StreamableToolCallEndpoint endpoint,
                           const ToolContext& /*t_ctx*/) override {
        return {endpoint, ""};
    }

    std::pair<std::shared_ptr<void>, std::string>
    WrapModel(const internal::core::ExecutionContext& /*ctx*/,
              std::shared_ptr<void> model,
              const ModelContext& /*mc*/) override {
        return {model, ""};
    }
};

// SetRunLocalValue sets a key-value pair that persists for the duration of the current agent Run().
// Aligned with Go: adk.SetRunLocalValue()
std::string SetRunLocalValue(const internal::core::ExecutionContext& ctx,
                             const std::string& key,
                             const std::any& value);

// GetRunLocalValue retrieves a value that was set during the current agent Run().
// Aligned with Go: adk.GetRunLocalValue()
std::tuple<std::any, bool, std::string> GetRunLocalValue(
    const internal::core::ExecutionContext& ctx,
    const std::string& key);

// DeleteRunLocalValue removes a value that was set during the current agent Run().
// Aligned with Go: adk.DeleteRunLocalValue()
std::string DeleteRunLocalValue(const internal::core::ExecutionContext& ctx,
                                const std::string& key);

// SendEvent sends a custom AgentEvent to the event stream during agent execution.
// Aligned with Go: adk.SendEvent()
std::string SendEvent(const internal::core::ExecutionContext& ctx,
                      std::shared_ptr<AgentEvent> event);

}  // namespace adk
}  // namespace eino

#endif  // EINO_CPP_ADK_HANDLER_H_
