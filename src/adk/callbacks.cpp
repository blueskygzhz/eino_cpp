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

#include "eino/adk/callbacks.h"
#include "eino/adk/stream_utils.h"
#include "eino/adk/tools.h"
#include "eino/adk/react.h"
#include "eino/compose/state.h"

namespace eino {
namespace adk {

// =============================================================================
// ChatModel Callbacks
// Aligns with: eino/adk/chatmodel.go:402-421
// =============================================================================

void ReactCallbackHandler::OnChatModelEnd(
    void* ctx,
    const callbacks::RunInfo& info,
    const schema::Message& output) {
    
    // Create event from message - aligns with chatmodel.go:406
    auto event = EventFromMessage(
        const_cast<schema::Message*>(&output),
        nullptr,
        schema::RoleType::kAssistant,
        "");
    
    // Send event - aligns with chatmodel.go:407
    if (generator_) {
        generator_->Send(event);
    }
}

void ReactCallbackHandler::OnChatModelEndWithStreamOutput(
    void* ctx,
    const callbacks::RunInfo& info,
    std::shared_ptr<schema::StreamReader<schema::Message>> output) {
    
    // Create event from stream - aligns with chatmodel.go:413-418
    auto event = EventFromMessage(
        nullptr,
        output.get(),
        schema::RoleType::kAssistant,
        "");
    
    // Send event - aligns with chatmodel.go:419
    if (generator_) {
        generator_->Send(event);
    }
}

// =============================================================================
// Tool Callbacks
// Aligns with: eino/adk/chatmodel.go:423-462
// =============================================================================

void ReactCallbackHandler::OnToolEnd(
    void* ctx,
    const callbacks::RunInfo& info,
    const std::string& tool_response,
    const std::string& tool_call_id) {
    
    // Create tool message - aligns with chatmodel.go:426-427
    auto msg = schema::ToolMessage(tool_response, tool_call_id, info.name);
    
    // Create event - aligns with chatmodel.go:428
    auto event = EventFromMessage(&msg, nullptr, schema::RoleType::kTool, info.name);
    
    // Pop tool generated action - aligns with chatmodel.go:430-431
    auto action = PopToolGenAction(ctx, info.name);
    if (action) {
        event->action = action;
    }
    
    // Check if this is return-directly tool - aligns with chatmodel.go:433-439
    auto [return_directly_id, has_return_directly] = GetReturnDirectlyToolCallID(ctx);
    if (has_return_directly && return_directly_id == tool_call_id) {
        // Store event to send after all tools complete
        // Aligns with chatmodel.go:436
        return_directly_tool_event_.store(event.get());
    } else {
        // Send event immediately - aligns with chatmodel.go:438
        if (generator_) {
            generator_->Send(event);
        }
    }
}

void ReactCallbackHandler::OnToolEndWithStreamOutput(
    void* ctx,
    const callbacks::RunInfo& info,
    std::shared_ptr<schema::StreamReader<std::string>> output,
    const std::string& tool_call_id) {
    
    // Convert string stream to message stream
    auto msg_stream = schema::StreamReaderWithConvert<std::string, schema::Message>(
        output,
        [tool_call_id, tool_name = info.name](const std::string& response) {
            return std::make_tuple(
                schema::ToolMessage(response, tool_call_id, tool_name),
                std::string("")  // no error
            );
        });
    
    // Create event - aligns with chatmodel.go:451
    auto event = EventFromMessage(nullptr, msg_stream.get(), schema::RoleType::kTool, info.name);
    
    // Check if this is return-directly tool - aligns with chatmodel.go:453-459
    auto [return_directly_id, has_return_directly] = GetReturnDirectlyToolCallID(ctx);
    if (has_return_directly && return_directly_id == tool_call_id) {
        // Store event to send after all tools complete
        return_directly_tool_event_.store(event.get());
    } else {
        // Send event immediately
        if (generator_) {
            generator_->Send(event);
        }
    }
}

// =============================================================================
// ToolsNode Callbacks
// Aligns with: eino/adk/chatmodel.go:464-478
// =============================================================================

void ReactCallbackHandler::SendReturnDirectlyToolEvent() {
    // Aligns with: sendReturnDirectlyToolEvent in chatmodel.go:464-468
    auto event = return_directly_tool_event_.load();
    if (event && generator_) {
        generator_->Send(std::shared_ptr<AgentEvent>(event));
    }
}

void ReactCallbackHandler::OnToolsNodeEnd(
    void* ctx,
    const callbacks::RunInfo& info,
    const std::vector<schema::Message>& messages) {
    
    // Send return-directly tool event if stored - aligns with chatmodel.go:471
    SendReturnDirectlyToolEvent();
}

void ReactCallbackHandler::OnToolsNodeEndWithStreamOutput(
    void* ctx,
    const callbacks::RunInfo& info,
    std::shared_ptr<schema::StreamReader<std::vector<schema::Message>>> output) {
    
    // Send return-directly tool event if stored - aligns with chatmodel.go:476
    SendReturnDirectlyToolEvent();
}

// =============================================================================
// Graph Error Callback
// Aligns with: eino/adk/chatmodel.go:489-514
// =============================================================================

void ReactCallbackHandler::OnGraphError(
    void* ctx,
    const callbacks::RunInfo& info,
    const std::exception& error) {
    
    // Try to extract interrupt info - aligns with chatmodel.go:492-496
    // For now, just send error event
    // Full interrupt handling requires compose::ExtractInterruptInfo
    
    auto error_event = std::make_shared<AgentEvent>();
    error_event->agent_name = agent_name_;
    error_event->error_msg = error.what();
    
    if (generator_) {
        generator_->Send(error_event);
    }
}

// =============================================================================
// GenReactCallbacks - Create compose option with callbacks
// Aligns with: eino/adk/chatmodel.go:516-540
// =============================================================================

compose::Option GenReactCallbacks(
    const std::string& agent_name,
    std::shared_ptr<AsyncGenerator<std::shared_ptr<AgentEvent>>> generator,
    bool enable_streaming,
    MockStore* store) {
    
    // Create callback handler - aligns with chatmodel.go:521
    auto handler = std::make_shared<ReactCallbackHandler>(
        generator, agent_name, enable_streaming, store);
    
    // Create ChatModel callback handler - aligns with chatmodel.go:523-526
    auto cm_handler = callbacks::HandlerBuilder()
        .WithOnEnd([handler](const callbacks::RunInfo& info, const callbacks::CallbackOutput& output) {
            // Extract message from output
            auto msg = output.output.get<schema::Message>();
            handler->OnChatModelEnd(nullptr, info, msg);
        })
        .WithOnEndWithStreamOutput([handler](const callbacks::RunInfo& info, const callbacks::CallbackOutput& output) {
            // Extract stream reader from output
            auto stream = output.output.get<std::shared_ptr<schema::StreamReader<schema::Message>>>();
            handler->OnChatModelEndWithStreamOutput(nullptr, info, stream);
        })
        .Build();
    
    // Create Tool callback handler - aligns with chatmodel.go:527-530
    auto tool_handler = callbacks::HandlerBuilder()
        .WithOnEnd([handler](const callbacks::RunInfo& info, const callbacks::CallbackOutput& output) {
            // Extract tool response and call ID
            auto response = output.output.get<std::string>();
            auto call_id = output.extra.at("tool_call_id").get<std::string>();
            handler->OnToolEnd(nullptr, info, response, call_id);
        })
        .WithOnEndWithStreamOutput([handler](const callbacks::RunInfo& info, const callbacks::CallbackOutput& output) {
            auto stream = output.output.get<std::shared_ptr<schema::StreamReader<std::string>>>();
            auto call_id = output.extra.at("tool_call_id").get<std::string>();
            handler->OnToolEndWithStreamOutput(nullptr, info, stream, call_id);
        })
        .Build();
    
    // Create ToolsNode callback handler - aligns with chatmodel.go:531-534
    auto tools_node_handler = callbacks::HandlerBuilder()
        .WithOnEnd([handler](const callbacks::RunInfo& info, const callbacks::CallbackOutput& output) {
            auto messages = output.output.get<std::vector<schema::Message>>();
            handler->OnToolsNodeEnd(nullptr, info, messages);
        })
        .WithOnEndWithStreamOutput([handler](const callbacks::RunInfo& info, const callbacks::CallbackOutput& output) {
            auto stream = output.output.get<std::shared_ptr<schema::StreamReader<std::vector<schema::Message>>>>();
            handler->OnToolsNodeEndWithStreamOutput(nullptr, info, stream);
        })
        .Build();
    
    // Create Graph error handler - aligns with chatmodel.go:535
    auto graph_handler = callbacks::HandlerBuilder()
        .WithOnError([handler](const callbacks::RunInfo& info, const std::string& error) {
            handler->OnGraphError(nullptr, info, std::runtime_error(error));
        })
        .Build();
    
    // Combine all handlers and return compose option - aligns with chatmodel.go:537-539
    // Note: Actual implementation depends on compose::WithCallbacks API
    return compose::WithCallbacks({cm_handler, tool_handler, tools_node_handler, graph_handler});
}

}  // namespace adk
}  // namespace eino
