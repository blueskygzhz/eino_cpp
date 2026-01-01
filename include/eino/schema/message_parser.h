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

#ifndef EINO_CPP_SCHEMA_MESSAGE_PARSER_H_
#define EINO_CPP_SCHEMA_MESSAGE_PARSER_H_

#include "types.h"
#include <string>
#include <vector>
#include <memory>

namespace eino {
namespace schema {

// MessageParser provides utilities to parse and manipulate messages
class MessageParser {
public:
    // GetToolCalls extracts all tool calls from a message
    // Returns empty vector if message has no tool calls
    static std::vector<ToolCall> GetToolCalls(const Message& msg);
    
    // GetToolCall extracts a specific tool call by index
    // Returns empty ToolCall if index out of bounds
    static ToolCall GetToolCall(const Message& msg, size_t index);
    
    // HasToolCalls checks if message contains any tool calls
    static bool HasToolCalls(const Message& msg);
    
    // GetToolCallCount returns the number of tool calls in message
    static size_t GetToolCallCount(const Message& msg);
    
    // GetTextContent extracts the text content from a message
    // Returns empty string if no text content
    static std::string GetTextContent(const Message& msg);
    
    // GetContent extracts the main content from a message
    // Can be text, multimodal content, or tool calls
    static std::string GetContent(const Message& msg);
    
    // GetReasoningContent extracts reasoning content (if available)
    static std::string GetReasoningContent(const Message& msg);
    
    // IsUserMessage checks if message is from user role
    static bool IsUserMessage(const Message& msg);
    
    // IsAssistantMessage checks if message is from assistant role
    static bool IsAssistantMessage(const Message& msg);
    
    // IsToolMessage checks if message is from tool role
    static bool IsToolMessage(const Message& msg);
    
    // GetRole safely gets the message role
    static RoleType GetRole(const Message& msg);
    
    // SetContent sets the text content of a message
    static void SetContent(Message& msg, const std::string& content);
    
    // AddToolCall adds a tool call to a message
    static void AddToolCall(Message& msg, const ToolCall& tool_call);
    
    // ClearToolCalls removes all tool calls from a message
    static void ClearToolCalls(Message& msg);
};

} // namespace schema
} // namespace eino

#endif // EINO_CPP_SCHEMA_MESSAGE_PARSER_H_
