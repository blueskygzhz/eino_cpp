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

#include "eino/schema/message_format.h"
#include <regex>
#include <sstream>
#include <stdexcept>

namespace eino {
namespace schema {

// Simple Python f-string formatter
// Supports {var} and {var:format_spec}
std::string FormatFString(
    const std::string& template_str,
    const std::map<std::string, json>& params) {
    
    std::string result = template_str;
    std::regex pattern(R"(\{([^:}]+)(?::([^}]+))?\})");
    std::smatch match;
    std::string::const_iterator search_start(result.cbegin());
    
    std::ostringstream out;
    size_t last_pos = 0;
    
    while (std::regex_search(search_start, result.cend(), match, pattern)) {
        size_t match_pos = match.position(0) + (search_start - result.cbegin());
        
        // Append text before match
        out << result.substr(last_pos, match_pos - last_pos);
        
        std::string var_name = match[1].str();
        
        // Look up variable
        auto it = params.find(var_name);
        if (it == params.end()) {
            throw std::runtime_error("Variable not found in params: " + var_name);
        }
        
        // Convert to string
        if (it->second.is_string()) {
            out << it->second.get<std::string>();
        } else if (it->second.is_number()) {
            out << it->second.dump();
        } else if (it->second.is_boolean()) {
            out << (it->second.get<bool>() ? "true" : "false");
        } else {
            out << it->second.dump();
        }
        
        last_pos = match_pos + match.length(0);
        search_start = result.cbegin() + last_pos;
    }
    
    // Append remaining text
    out << result.substr(last_pos);
    
    return out.str();
}

// Simple Go template formatter
// Supports {{.Var}} style
std::string FormatGoTemplate(
    const std::string& template_str,
    const std::map<std::string, json>& params) {
    
    std::string result = template_str;
    std::regex pattern(R"(\{\{\.([^}]+)\}\})");
    std::smatch match;
    
    std::ostringstream out;
    size_t last_pos = 0;
    std::string::const_iterator search_start(result.cbegin());
    
    while (std::regex_search(search_start, result.cend(), match, pattern)) {
        size_t match_pos = match.position(0) + (search_start - result.cbegin());
        
        out << result.substr(last_pos, match_pos - last_pos);
        
        std::string var_name = match[1].str();
        
        auto it = params.find(var_name);
        if (it == params.end()) {
            throw std::runtime_error("Variable not found in params: " + var_name);
        }
        
        if (it->second.is_string()) {
            out << it->second.get<std::string>();
        } else {
            out << it->second.dump();
        }
        
        last_pos = match_pos + match.length(0);
        search_start = result.cbegin() + last_pos;
    }
    
    out << result.substr(last_pos);
    
    return out.str();
}

// Simple Jinja2 formatter
// Supports {{var}} style (subset of Jinja2)
std::string FormatJinja2(
    const std::string& template_str,
    const std::map<std::string, json>& params) {
    
    // Basic implementation - just support {{var}}
    // For production, integrate with a proper Jinja2 library like inja
    std::string result = template_str;
    std::regex pattern(R"(\{\{([^}]+)\}\})");
    std::smatch match;
    
    std::ostringstream out;
    size_t last_pos = 0;
    std::string::const_iterator search_start(result.cbegin());
    
    while (std::regex_search(search_start, result.cend(), match, pattern)) {
        size_t match_pos = match.position(0) + (search_start - result.cbegin());
        
        out << result.substr(last_pos, match_pos - last_pos);
        
        std::string var_expr = match[1].str();
        
        // Trim whitespace
        var_expr.erase(0, var_expr.find_first_not_of(" \t\n\r"));
        var_expr.erase(var_expr.find_last_not_of(" \t\n\r") + 1);
        
        auto it = params.find(var_expr);
        if (it == params.end()) {
            throw std::runtime_error("Variable not found in params: " + var_expr);
        }
        
        if (it->second.is_string()) {
            out << it->second.get<std::string>();
        } else {
            out << it->second.dump();
        }
        
        last_pos = match_pos + match.length(0);
        search_start = result.cbegin() + last_pos;
    }
    
    out << result.substr(last_pos);
    
    return out.str();
}

std::string FormatContent(
    const std::string& content,
    const std::map<std::string, json>& params,
    FormatType format_type) {
    
    switch (format_type) {
        case FormatType::kFString:
            return FormatFString(content, params);
        case FormatType::kGoTemplate:
            return FormatGoTemplate(content, params);
        case FormatType::kJinja2:
            return FormatJinja2(content, params);
        default:
            throw std::runtime_error("Unknown format type");
    }
}

std::vector<ChatMessagePart> FormatMultiContent(
    const std::vector<ChatMessagePart>& multi_content,
    const std::map<std::string, json>& params,
    FormatType format_type) {
    
    std::vector<ChatMessagePart> result = multi_content;
    
    for (auto& part : result) {
        switch (part.type) {
            case ChatMessagePartType::kText:
                part.text = FormatContent(part.text, params, format_type);
                break;
            case ChatMessagePartType::kImageURL:
                if (part.image_url && !part.image_url->url.empty()) {
                    part.image_url->url = FormatContent(part.image_url->url, params, format_type);
                }
                break;
            case ChatMessagePartType::kAudioURL:
                if (part.audio_url && !part.audio_url->url.empty()) {
                    part.audio_url->url = FormatContent(part.audio_url->url, params, format_type);
                }
                break;
            case ChatMessagePartType::kVideoURL:
                if (part.video_url && !part.video_url->url.empty()) {
                    part.video_url->url = FormatContent(part.video_url->url, params, format_type);
                }
                break;
            case ChatMessagePartType::kFileURL:
                if (part.file_url && !part.file_url->url.empty()) {
                    part.file_url->url = FormatContent(part.file_url->url, params, format_type);
                }
                break;
        }
    }
    
    return result;
}

std::vector<MessageInputPart> FormatUserInputMultiContent(
    const std::vector<MessageInputPart>& user_input_multi_content,
    const std::map<std::string, json>& params,
    FormatType format_type) {
    
    std::vector<MessageInputPart> result = user_input_multi_content;
    
    for (auto& part : result) {
        switch (part.type) {
            case ChatMessagePartType::kText:
                part.text = FormatContent(part.text, params, format_type);
                break;
            case ChatMessagePartType::kImageURL:
                if (part.image && part.image->common.url != nullptr) {
                    std::string formatted = FormatContent(*part.image->common.url, params, format_type);
                    delete part.image->common.url;
                    part.image->common.url = new std::string(formatted);
                }
                break;
            case ChatMessagePartType::kAudioURL:
                if (part.audio && part.audio->common.url != nullptr) {
                    std::string formatted = FormatContent(*part.audio->common.url, params, format_type);
                    delete part.audio->common.url;
                    part.audio->common.url = new std::string(formatted);
                }
                break;
            case ChatMessagePartType::kVideoURL:
                if (part.video && part.video->common.url != nullptr) {
                    std::string formatted = FormatContent(*part.video->common.url, params, format_type);
                    delete part.video->common.url;
                    part.video->common.url = new std::string(formatted);
                }
                break;
            case ChatMessagePartType::kFileURL:
                if (part.file && part.file->common.url != nullptr) {
                    std::string formatted = FormatContent(*part.file->common.url, params, format_type);
                    delete part.file->common.url;
                    part.file->common.url = new std::string(formatted);
                }
                break;
        }
    }
    
    return result;
}

std::vector<Message> MessageTemplate::Format(
    const callbacks::CallbackManager* ctx,
    const std::map<std::string, json>& params,
    FormatType format_type) const {
    
    Message result = message_;
    
    // Format content
    if (!result.content.empty()) {
        result.content = FormatContent(result.content, params, format_type);
    }
    
    // Format deprecated MultiContent
    if (!result.multi_content.empty()) {
        result.multi_content = FormatMultiContent(result.multi_content, params, format_type);
    }
    
    // Format UserInputMultiContent
    if (!result.user_input_multi_content.empty()) {
        result.user_input_multi_content = FormatUserInputMultiContent(
            result.user_input_multi_content, params, format_type);
    }
    
    return {result};
}

std::vector<Message> MessagesPlaceholderTemplate::Format(
    const callbacks::CallbackManager* ctx,
    const std::map<std::string, json>& params,
    FormatType format_type) const {
    
    auto it = params.find(key_);
    if (it == params.end()) {
        if (optional_) {
            return {};
        }
        throw std::runtime_error("Message placeholder not found: " + key_);
    }
    
    const auto& value = it->second;
    
    // Expect array of messages
    if (!value.is_array()) {
        throw std::runtime_error(
            "Message placeholder '" + key_ + "' must be an array of messages");
    }
    
    std::vector<Message> messages;
    for (const auto& msg_json : value) {
        Message msg;
        
        // Parse role
        if (msg_json.contains("role")) {
            std::string role_str = msg_json["role"].get<std::string>();
            if (role_str == "user") {
                msg.role = RoleType::kUser;
            } else if (role_str == "assistant") {
                msg.role = RoleType::kAssistant;
            } else if (role_str == "system") {
                msg.role = RoleType::kSystem;
            } else if (role_str == "tool") {
                msg.role = RoleType::kTool;
            }
        }
        
        // Parse content
        if (msg_json.contains("content")) {
            msg.content = msg_json["content"].get<std::string>();
        }
        
        // Parse other fields as needed
        if (msg_json.contains("name")) {
            msg.name = msg_json["name"].get<std::string>();
        }
        
        messages.push_back(msg);
    }
    
    return messages;
}

} // namespace schema
} // namespace eino
