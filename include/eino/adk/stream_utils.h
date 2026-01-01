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

#ifndef EINO_CPP_ADK_STREAM_UTILS_H_
#define EINO_CPP_ADK_STREAM_UTILS_H_

// Stream Mode Support Utilities
// Aligns with eino adk stream handling functions
// Reference: eino/adk/interface.go, utils.go

#include "types.h"
#include "../schema/types.h"
#include <string>
#include <memory>

namespace eino {
namespace adk {

// ============================================================================
// Stream Mode Support Functions (Aligns with eino adk)
// ============================================================================

// EventFromMessage creates an AgentEvent from a Message or MessageStream
// Aligns with eino adk.EventFromMessage (interface.go:33-51)
//
// Go signature:
//   func EventFromMessage(msg Message, msgStream MessageStream,
//       role schema.RoleType, toolName string) *AgentEvent
//
// Parameters:
//   msg: Single message (non-streaming mode), can be nullptr if msg_stream is set
//   msg_stream: Message stream (streaming mode), can be nullptr if msg is set
//   role: Message role (Assistant or Tool)
//   tool_name: Tool name (only used when role is Tool)
//
// Returns:
//   AgentEvent with properly configured MessageVariant output
//
// Usage:
//   // Non-streaming
//   auto event = EventFromMessage(message, nullptr, schema::RoleType::Assistant, "");
//
//   // Streaming
//   auto event = EventFromMessage(nullptr, msg_stream, schema::RoleType::Assistant, "");
std::shared_ptr<AgentEvent> EventFromMessage(
    Message msg,
    MessageStream msg_stream,
    schema::RoleType role,
    const std::string& tool_name = "");

// GetMessageFromVariant extracts the final Message from a MessageVariant
// Aligns with eino adk.MessageVariant.GetMessage (interface.go:106-120)
//
// Go signature:
//   func (mv *MessageVariant) GetMessage() (Message, error)
//
// If streaming: concatenates all chunks from MessageStream into a single Message
// If not streaming: returns the single Message directly
//
// Parameters:
//   variant: MessageVariant containing either a Message or MessageStream
//
// Returns:
//   Concatenated or single Message, nullptr on error
//
// Usage:
//   auto final_message = GetMessageFromVariant(event->output->message_output.get());
Message GetMessageFromVariant(const MessageVariant* variant);

// SetAutomaticClose sets automatic close on MessageStream in AgentEvent
// Aligns with eino adk.setAutomaticClose (utils.go:83-90)
//
// Go implementation:
//   func setAutomaticClose(e *AgentEvent) {
//       if e.Output == nil || e.Output.MessageOutput == nil || !e.Output.MessageOutput.IsStreaming {
//           return
//       }
//       e.Output.MessageOutput.MessageStream.SetAutomaticClose()
//   }
//
// Ensures that even if events are not processed, the MessageStream will be closed.
// This is critical for preventing resource leaks in streaming mode.
//
// Usage:
//   SetAutomaticClose(event);  // Ensure stream is closed automatically
void SetAutomaticClose(std::shared_ptr<AgentEvent> event);

// CopyAgentEvent creates a safe copy of AgentEvent
// Aligns with eino adk.copyAgentEvent (utils.go:154-193)
//
// Go signature:
//   func copyAgentEvent(ae *AgentEvent) *AgentEvent
//
// If MessageVariant is streaming, the MessageStream will be copied.
// This ensures:
// - Each copy has exclusive MessageStream
// - Safe to receive from MessageStream
// - Message chunks are not copied (shared pointers)
//
// Returns:
//   Deep copy of AgentEvent with exclusive MessageStream (if streaming)
//
// Usage:
//   auto copied = CopyAgentEvent(original_event);
//   SetAutomaticClose(copied);  // Often used together
std::shared_ptr<AgentEvent> CopyAgentEvent(const std::shared_ptr<AgentEvent>& event);

// GetMessageFromWrappedEvent extracts Message from AgentEvent
// Aligns with eino adk.getMessageFromWrappedEvent (utils.go:93-135)
//
// Go signature:
//   func getMessageFromWrappedEvent(event *AgentEvent) (Message, error)
//
// Handles both streaming and non-streaming MessageVariant.
// For streaming: concatenates all chunks from stream.
// For non-streaming: returns message directly.
//
// Parameters:
//   event: AgentEvent to extract message from
//
// Returns:
//   Final message, nullptr if no message or error
//
// Usage:
//   auto msg = GetMessageFromWrappedEvent(event);
//   if (msg) {
//       std::cout << msg->content << std::endl;
//   }
Message GetMessageFromWrappedEvent(const std::shared_ptr<AgentEvent>& event);

}  // namespace adk
}  // namespace eino

#endif  // EINO_CPP_ADK_STREAM_UTILS_H_
