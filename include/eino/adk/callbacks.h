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

#ifndef EINO_CPP_ADK_CALLBACKS_H_
#define EINO_CPP_ADK_CALLBACKS_H_

// Agent Callbacks for ReAct execution
// ====================================
// Aligns with eino/adk/chatmodel.go:393-540
// Provides callbacks to emit AgentEvents during ReAct Graph execution

#include "types.h"
#include "async_iterator.h"
#include "../callbacks/interface.h"
#include "../compose/option.h"
#include "../schema/types.h"
#include <memory>
#include <atomic>
#include <string>

namespace eino {
namespace adk {

// Forward declarations
class MockStore;

// CallbackHandler for ReAct agent execution
// Aligns with: cbHandler in eino/adk/chatmodel.go:393-478
class ReactCallbackHandler {
public:
    ReactCallbackHandler(
        std::shared_ptr<AsyncGenerator<std::shared_ptr<AgentEvent>>> generator,
        const std::string& agent_name,
        bool enable_streaming,
        MockStore* store)
        : generator_(generator),
          agent_name_(agent_name),
          enable_streaming_(enable_streaming),
          store_(store) {}

    ~ReactCallbackHandler() = default;

    // ChatModel callbacks
    // Aligns with: onChatModelEnd in chatmodel.go:402-408
    void OnChatModelEnd(
        void* ctx,
        const callbacks::RunInfo& info,
        const schema::Message& output);

    // Aligns with: onChatModelEndWithStreamOutput in chatmodel.go:410-421
    void OnChatModelEndWithStreamOutput(
        void* ctx,
        const callbacks::RunInfo& info,
        std::shared_ptr<schema::StreamReader<schema::Message>> output);

    // Tool callbacks
    // Aligns with: onToolEnd in chatmodel.go:423-441
    void OnToolEnd(
        void* ctx,
        const callbacks::RunInfo& info,
        const std::string& tool_response,
        const std::string& tool_call_id);

    // Aligns with: onToolEndWithStreamOutput in chatmodel.go:443-462
    void OnToolEndWithStreamOutput(
        void* ctx,
        const callbacks::RunInfo& info,
        std::shared_ptr<schema::StreamReader<std::string>> output,
        const std::string& tool_call_id);

    // ToolsNode callbacks
    // Aligns with: onToolsNodeEnd in chatmodel.go:470-473
    void OnToolsNodeEnd(
        void* ctx,
        const callbacks::RunInfo& info,
        const std::vector<schema::Message>& messages);

    // Aligns with: onToolsNodeEndWithStreamOutput in chatmodel.go:475-478
    void OnToolsNodeEndWithStreamOutput(
        void* ctx,
        const callbacks::RunInfo& info,
        std::shared_ptr<schema::StreamReader<std::vector<schema::Message>>> output);

    // Graph error callback
    // Aligns with: onGraphError in chatmodel.go:489-514
    void OnGraphError(
        void* ctx,
        const callbacks::RunInfo& info,
        const std::exception& error);

private:
    // Helper to send return-directly tool event
    // Aligns with: sendReturnDirectlyToolEvent in chatmodel.go:464-468
    void SendReturnDirectlyToolEvent();

    std::shared_ptr<AsyncGenerator<std::shared_ptr<AgentEvent>>> generator_;
    std::string agent_name_;
    bool enable_streaming_;
    MockStore* store_;
    
    // Stores return-directly tool event to send after all tools complete
    // Aligns with: returnDirectlyToolEvent atomic.Value in chatmodel.go:399
    std::atomic<AgentEvent*> return_directly_tool_event_{nullptr};
};

// Generate React callbacks compose option
// Aligns with: genReactCallbacks in eino/adk/chatmodel.go:516-540
compose::Option GenReactCallbacks(
    const std::string& agent_name,
    std::shared_ptr<AsyncGenerator<std::shared_ptr<AgentEvent>>> generator,
    bool enable_streaming,
    MockStore* store);

}  // namespace adk
}  // namespace eino

#endif  // EINO_CPP_ADK_CALLBACKS_H_
