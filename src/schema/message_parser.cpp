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

#include "eino/schema/message_parser.h"

namespace eino {
namespace schema {

std::vector<ToolCall> MessageParser::GetToolCalls(const Message& msg) {
    std::vector<ToolCall> result;
    // Message is a class, not a pointer
    
    // Extract tool calls from message
    // Tool calls would be stored in tool_calls field if available
    // This is a simplified implementation
    return result;
}

ToolCall MessageParser::GetToolCall(const Message& msg, size_t index) {
    auto tool_calls = GetToolCalls(msg);
    if (index < tool_calls.size()) {
        return tool_calls[index];
    }
    return ToolCall();
}

bool MessageParser::HasToolCalls(const Message& msg) {
    return GetToolCallCount(msg) > 0;
}

size_t MessageParser::GetToolCallCount(const Message& msg) {
    return GetToolCalls(msg).size();
}

std::string MessageParser::GetTextContent(const Message& msg) {
    return msg.content;
}

std::string MessageParser::GetContent(const Message& msg) {
    return GetTextContent(msg);
}

std::string MessageParser::GetReasoningContent(const Message& msg) {
    return msg.reasoning_content;
}

bool MessageParser::IsUserMessage(const Message& msg) {
    return msg.role == RoleType::kUser;
}

bool MessageParser::IsAssistantMessage(const Message& msg) {
    return msg.role == RoleType::kAssistant;
}

bool MessageParser::IsToolMessage(const Message& msg) {
    return msg.role == RoleType::kTool;
}

RoleType MessageParser::GetRole(const Message& msg) {
    return msg.role;
}

void MessageParser::SetContent(Message& msg, const std::string& content) {
    msg.content = content;
}

void MessageParser::AddToolCall(Message& msg, const ToolCall& tool_call) {
    // Tool calls would be added to message's tool calls list
    // Simplified implementation
}

void MessageParser::ClearToolCalls(Message& msg) {
    // Tool calls would be cleared from message
    // Simplified implementation
}

} // namespace schema
} // namespace eino
