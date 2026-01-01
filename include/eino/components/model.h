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

#ifndef EINO_CPP_COMPONENTS_MODEL_H_
#define EINO_CPP_COMPONENTS_MODEL_H_

#include "../compose/runnable.h"
#include "../schema/types.h"
#include <vector>
#include <memory>

namespace eino {
namespace components {

// BaseChatModel defines the basic interface for chat models
// Input: vector of Message, Output: Message
class BaseChatModel : public compose::Runnable<std::vector<schema::Message>, schema::Message> {
public:
    virtual ~BaseChatModel() = default;
    
    // Generate a single message response
    virtual schema::Message Generate(
        std::shared_ptr<compose::Context> ctx,
        const std::vector<schema::Message>& input,
        const std::vector<compose::Option>& opts = std::vector<compose::Option>()) = 0;
};

// ChatModel is the legacy interface with BindTools
// Deprecated: Use ToolCallingChatModel instead
class ChatModel : public BaseChatModel {
public:
    virtual ~ChatModel() = default;
    
    // BindTools binds tools to the model
    // Note: This is not atomic and has concurrency issues
    virtual void BindTools(const std::vector<schema::ToolInfo>& tools) = 0;
};

// ToolCallingChatModel extends BaseChatModel with tool calling capabilities
// Provides WithTools method that returns a new instance instead of mutating state
class ToolCallingChatModel : public BaseChatModel {
public:
    virtual ~ToolCallingChatModel() = default;
    
    // WithTools returns a new instance with tools bound
    // This method does not modify the current instance
    virtual std::shared_ptr<ToolCallingChatModel> WithTools(
        const std::vector<schema::ToolInfo>& tools) = 0;
};

} // namespace components
} // namespace eino

#endif // EINO_CPP_COMPONENTS_MODEL_H_
