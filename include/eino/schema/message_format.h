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

#ifndef EINO_CPP_SCHEMA_MESSAGE_FORMAT_H_
#define EINO_CPP_SCHEMA_MESSAGE_FORMAT_H_

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>

#include "eino/schema/types.h"
#include "eino/callbacks/callback_manager.h"

namespace eino {
namespace schema {

// MessagesTemplate is the interface for message templates
// Used to render templates with variable substitution
//
// Example:
//   auto tpl = SystemMessage("You are {role}");
//   auto params = {{"role", "a helpful assistant"}};
//   auto msgs = tpl->Format(ctx, params, FormatType::kFString);
class MessagesTemplate {
public:
    virtual ~MessagesTemplate() = default;
    
    // Format renders the template with given parameters
    virtual std::vector<Message> Format(
        const callbacks::CallbackManager* ctx,
        const std::map<std::string, json>& params,
        FormatType format_type = FormatType::kFString) const = 0;
};

// MessageTemplate implements MessagesTemplate for single message
class MessageTemplate : public MessagesTemplate {
public:
    explicit MessageTemplate(const Message& msg) : message_(msg) {}
    
    std::vector<Message> Format(
        const callbacks::CallbackManager* ctx,
        const std::map<std::string, json>& params,
        FormatType format_type = FormatType::kFString) const override;

private:
    Message message_;
};

// MessagesPlaceholderTemplate replaces itself with messages from params
// Used in prompt chaining to inject message history
//
// Example:
//   auto placeholder = MessagesPlaceholder("history", false);
//   auto params = {
//       {"history", json::array({
//           {{"role", "user"}, {"content", "Hi"}},
//           {{"role", "assistant"}, {"content", "Hello!"}}
//       })}
//   };
//   auto msgs = placeholder->Format(ctx, params);
class MessagesPlaceholderTemplate : public MessagesTemplate {
public:
    MessagesPlaceholderTemplate(const std::string& key, bool optional)
        : key_(key), optional_(optional) {}
    
    std::vector<Message> Format(
        const callbacks::CallbackManager* ctx,
        const std::map<std::string, json>& params,
        FormatType format_type = FormatType::kFString) const override;

private:
    std::string key_;
    bool optional_;
};

// Factory function for MessagesPlaceholder
inline std::shared_ptr<MessagesTemplate> MessagesPlaceholder(
    const std::string& key, bool optional = false) {
    return std::make_shared<MessagesPlaceholderTemplate>(key, optional);
}

// FormatContent formats a string with given parameters and format type
// Supports FString, GoTemplate, and Jinja2 formats
std::string FormatContent(
    const std::string& content,
    const std::map<std::string, json>& params,
    FormatType format_type);

// FormatMultiContent formats deprecated ChatMessagePart array
std::vector<ChatMessagePart> FormatMultiContent(
    const std::vector<ChatMessagePart>& multi_content,
    const std::map<std::string, json>& params,
    FormatType format_type);

// FormatUserInputMultiContent formats user input multimodal parts
std::vector<MessageInputPart> FormatUserInputMultiContent(
    const std::vector<MessageInputPart>& user_input_multi_content,
    const std::map<std::string, json>& params,
    FormatType format_type);

// Helper functions for Python f-string format
// Supports {var}, {var:format}, etc.
std::string FormatFString(
    const std::string& template_str,
    const std::map<std::string, json>& params);

// Helper functions for Go template format
// Supports {{.Var}}, {{range .Items}}...{{end}}, etc.
std::string FormatGoTemplate(
    const std::string& template_str,
    const std::map<std::string, json>& params);

// Helper functions for Jinja2 template format
// Supports {{var}}, {% for item in items %}...{% endfor %}, etc.
// Note: For security, include/extends/import/from are disabled
std::string FormatJinja2(
    const std::string& template_str,
    const std::map<std::string, json>& params);

} // namespace schema
} // namespace eino

#endif // EINO_CPP_SCHEMA_MESSAGE_FORMAT_H_
