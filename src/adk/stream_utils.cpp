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

// Stream Mode Support Utilities
// Aligns with eino adk stream handling (interface.go, utils.go)

#include "../include/eino/adk/types.h"
#include "../include/eino/schema/types.h"
#include "../include/eino/schema/message_concat.h"
#include <memory>
#include <vector>

namespace eino {
namespace adk {

// ============================================================================
// EventFromMessage - Create AgentEvent from Message or MessageStream
// Aligns with eino adk.EventFromMessage (interface.go:33-51)
// ============================================================================

std::shared_ptr<AgentEvent> EventFromMessage(
    Message msg,
    MessageStream msg_stream,
    schema::RoleType role,
    const std::string& tool_name) {
    
    auto event = std::make_shared<AgentEvent>();
    event->output = std::make_shared<AgentOutput>();
    
    auto variant = std::make_shared<MessageVariant>();
    variant->is_streaming = (msg_stream != nullptr);
    variant->message = msg;
    variant->message_stream = msg_stream;
    variant->role = role;
    variant->tool_name = tool_name;
    
    event->output->message_output = variant;
    
    return event;
}

// ============================================================================
// GetMessageFromVariant - Extract final Message from MessageVariant
// Aligns with eino adk.MessageVariant.GetMessage (interface.go:106-120)
// ============================================================================

Message GetMessageFromVariant(const MessageVariant* variant) {
    if (!variant) {
        return nullptr;
    }
    
    if (variant->is_streaming) {
        // Streaming mode: concatenate all chunks
        if (!variant->message_stream) {
            return nullptr;
        }
        
        std::vector<Message> messages;
        Message chunk;
        while (variant->message_stream->Read(chunk)) {
            if (chunk) {
                messages.push_back(chunk);
            }
        }
        
        if (messages.empty()) {
            return nullptr;
        }
        
        // Use schema::ConcatMessages to merge all chunks
        return schema::ConcatMessages(messages);
        
    } else {
        // Non-streaming mode: return single message
        return variant->message;
    }
}

// ============================================================================
// SetAutomaticClose - Set automatic close on MessageStream
// Aligns with eino adk.setAutomaticClose (utils.go:83-90)
// ============================================================================

void SetAutomaticClose(std::shared_ptr<AgentEvent> event) {
    if (!event || !event->output || !event->output->message_output) {
        return;
    }
    
    auto msg_output = event->output->message_output;
    if (!msg_output->is_streaming || !msg_output->message_stream) {
        return;
    }
    
    // Set automatic close on stream
    // In C++ implementation, would use RAII or shared_ptr with custom deleter
    // to ensure stream is closed when last reference is destroyed
    msg_output->message_stream->SetAutomaticClose();
}

// ============================================================================
// CopyAgentEvent - Create a safe copy of AgentEvent
// Aligns with eino adk.copyAgentEvent (utils.go:154-193)
// ============================================================================

std::shared_ptr<AgentEvent> CopyAgentEvent(const std::shared_ptr<AgentEvent>& event) {
    if (!event) {
        return nullptr;
    }
    
    auto copied = std::make_shared<AgentEvent>();
    copied->agent_name = event->agent_name;
    copied->run_path = event->run_path;
    copied->error = event->error;
    copied->error_msg = event->error_msg;
    
    // Copy action (shallow copy is fine for action)
    copied->action = event->action;
    
    // Deep copy output if it contains MessageVariant
    if (event->output && event->output->message_output) {
        copied->output = std::make_shared<AgentOutput>();
        
        auto orig_variant = event->output->message_output;
        auto copied_variant = std::make_shared<MessageVariant>();
        
        copied_variant->is_streaming = orig_variant->is_streaming;
        copied_variant->role = orig_variant->role;
        copied_variant->tool_name = orig_variant->tool_name;
        
        if (orig_variant->is_streaming && orig_variant->message_stream) {
            // Copy stream to make each copy exclusive
            // Use schema::StreamReader::Copy(2) to create 2 copies
            auto streams = orig_variant->message_stream->Copy(2);
            if (streams.size() >= 2) {
                copied_variant->message_stream = streams[1];
                // Update original to use first copy
                const_cast<MessageVariant*>(orig_variant.get())->message_stream = streams[0];
            }
        } else {
            // Non-streaming: just copy message pointer (shallow)
            copied_variant->message = orig_variant->message;
        }
        
        copied->output->message_output = copied_variant;
        
        // Copy customized output if exists
        copied->output->customized_output = event->output->customized_output;
    }
    
    return copied;
}

// ============================================================================
// GetMessageFromWrappedEvent - Extract Message from AgentEvent
// Aligns with eino adk.getMessageFromWrappedEvent (utils.go:93-135)
// ============================================================================

Message GetMessageFromWrappedEvent(const std::shared_ptr<AgentEvent>& event) {
    if (!event) {
        return nullptr;
    }
    
    if (event->HasError()) {
        return nullptr;
    }
    
    if (!event->output || !event->output->message_output) {
        return nullptr;
    }
    
    auto msg_output = event->output->message_output;
    
    if (msg_output->is_streaming) {
        // Streaming mode: need to concatenate
        if (!msg_output->message_stream) {
            return nullptr;
        }
        
        std::vector<Message> chunks;
        Message chunk;
        while (msg_output->message_stream->Read(chunk)) {
            if (chunk) {
                chunks.push_back(chunk);
            }
        }
        
        if (chunks.empty()) {
            return nullptr;
        }
        
        return schema::ConcatMessages(chunks);
        
    } else {
        // Non-streaming mode: return message directly
        return msg_output->message;
    }
}

}  // namespace adk
}  // namespace eino
