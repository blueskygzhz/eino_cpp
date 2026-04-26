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

#ifndef EINO_CPP_ADK_WRAPPERS_H_
#define EINO_CPP_ADK_WRAPPERS_H_

// Model and Tool wrapper implementations for ChatModelAgentMiddleware.
// Aligned with Go: adk/wrappers.go
//
// These wrappers intercept model and tool calls to:
// 1. Inject callbacks for models that don't support them natively
// 2. Apply state management (before/after model hooks)
// 3. Send events to the agent's event stream
// 4. Apply retry logic for model calls

#include <memory>
#include <string>
#include <vector>

#include "eino/adk/handler.h"
#include "eino/adk/types.h"
#include "eino/internal/core/address.h"
#include "eino/schema/types.h"

namespace eino {
namespace adk {

// ModelWrapperConfig holds configuration for building model wrappers.
// Aligned with Go: adk.modelWrapperConfig
struct ModelWrapperConfig {
    std::vector<std::shared_ptr<ChatModelAgentMiddleware>> handlers;
    std::shared_ptr<ModelRetryConfig> retry_config;
    std::vector<std::shared_ptr<schema::ToolInfo>> tool_infos;
};

// BuildModelWrappers constructs the full model wrapper chain.
// Aligned with Go: adk.buildModelWrappers()
//
// The wrapper chain is (outermost to innermost):
// 1. RetryWrapper (if retry config is set)
// 2. EventSenderWrapper (if no user-provided event sender)
// 3. User handlers (in reverse order, so first handler is outermost)
// 4. CallbackInjectionWrapper (if model doesn't support callbacks)
// 5. StateModelWrapper (manages before/after model hooks)
std::shared_ptr<void> BuildModelWrappers(
    std::shared_ptr<void> model,  // BaseChatModel
    const ModelWrapperConfig& config);

// NewEventSenderModelWrapper returns a ChatModelAgentMiddleware that sends model response events.
// Aligned with Go: adk.NewEventSenderModelWrapper()
//
// By default, the framework applies this wrapper after all user middlewares, so events contain
// modified messages. To send events with original (unmodified) output, pass this as a Handler
// after the modifying middleware (placing it innermost in the wrapper chain).
std::shared_ptr<ChatModelAgentMiddleware> NewEventSenderModelWrapper();

// EventFromMessage creates an AgentEvent from a message or message stream.
// Aligned with Go: adk.EventFromMessage()
std::shared_ptr<AgentEvent> EventFromMessage(
    schema::Message* msg,
    std::shared_ptr<schema::StreamReader<schema::Message*>> msg_stream,
    schema::RoleType role,
    const std::string& tool_name);

}  // namespace adk
}  // namespace eino

#endif  // EINO_CPP_ADK_WRAPPERS_H_
