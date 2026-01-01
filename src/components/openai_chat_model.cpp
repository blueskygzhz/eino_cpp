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

#include "eino/components/openai_chat_model.h"
#include "eino/components/model_with_tools.h"
#include <stdexcept>

namespace eino {
namespace components {

// Constructor
OpenAIChatModel::OpenAIChatModel(
    std::shared_ptr<compose::Context> ctx,
    std::shared_ptr<OpenAIChatModelConfig> config)
    : ctx_(ctx), config_(config) {
    
    if (!config_) {
        throw std::invalid_argument("OpenAIChatModel: config cannot be null");
    }
    
    if (config_->api_key.empty()) {
        throw std::invalid_argument("OpenAIChatModel: API key is required");
    }
}

// CloneWithToolsImpl - implements immutable WithTools pattern
// Aligns with eino/components/model/interface.go:55 (WithTools)
// Key points:
// 1. Deep copy config (ensures isolation)
// 2. Set tools on NEW config (not modifying current)
// 3. Convert tools to JSON format
// 4. Return NEW instance
std::shared_ptr<OpenAIChatModel> OpenAIChatModel::CloneWithToolsImpl(
    const std::vector<schema::ToolInfo>& tools) {
    
    // 1. Deep copy configuration
    // This is CRITICAL for immutability - we must not modify the current instance
    auto new_config = std::make_shared<OpenAIChatModelConfig>(*config_);
    
    // 2. Set tools in new config
    new_config->tools = tools;
    
    // 3. Convert tools to OpenAI JSON format
    // OpenAI expects tools in specific format:
    // [
    //   {
    //     "type": "function",
    //     "function": {
    //       "name": "...",
    //       "description": "...",
    //       "parameters": { ... }
    //     }
    //   }
    // ]
    if (!tools.empty()) {
        new_config->tools_json = ConvertToolsToJSON(tools);
    } else {
        new_config->tools_json = nlohmann::json::array();
    }
    
    // 4. Create and return new instance
    // The current instance remains unchanged
    return std::make_shared<OpenAIChatModel>(ctx_, new_config);
}

// Generate - synchronous generation
// Aligns with eino/components/model/interface.go:31-32
schema::Message OpenAIChatModel::Generate(
    std::shared_ptr<compose::Context> ctx,
    const std::vector<schema::Message>& input,
    const std::vector<compose::Option>& opts) {
    
    // TODO: Implement actual OpenAI API call
    // This is a placeholder showing the structure
    
    // 1. Build request JSON
    auto request_json = BuildRequestJSON(input, opts);
    
    // 2. Make API request
    // auto response_json = MakeAPIRequest("/chat/completions", request_json);
    
    // 3. Parse response
    // return ParseResponseJSON(response_json);
    
    // Placeholder implementation
    schema::Message result;
    result.role = schema::Role::kAssistant;
    result.content = "[OpenAI response placeholder]";
    
    // If tools are bound, demonstrate tool calling structure
    if (!config_->tools.empty()) {
        // Model might return tool calls
        schema::ToolCall tool_call;
        tool_call.id = "call_abc123";
        tool_call.type = "function";
        tool_call.function.name = config_->tools[0].name;
        tool_call.function.arguments = "{}";
        
        result.tool_calls.push_back(tool_call);
    }
    
    return result;
}

// Stream - streaming generation
// Aligns with eino/components/model/interface.go:33-34
std::shared_ptr<schema::StreamReader<schema::Message>> OpenAIChatModel::Stream(
    std::shared_ptr<compose::Context> ctx,
    const std::vector<schema::Message>& input,
    const std::vector<compose::Option>& opts) {
    
    // TODO: Implement actual streaming OpenAI API call
    // This is a placeholder showing the structure
    
    // For now, return a simple stream with a single message
    auto message = Generate(ctx, input, opts);
    
    // Create a simple stream reader (placeholder)
    // In real implementation, this would stream chunks from the API
    class SimpleMessageStream : public schema::StreamReader<schema::Message> {
    public:
        explicit SimpleMessageStream(const schema::Message& msg)
            : message_(msg), read_(false) {}
        
        bool Read(schema::Message& value) override {
            if (!read_) {
                value = message_;
                read_ = true;
                return true;
            }
            return false;
        }
        
    private:
        schema::Message message_;
        bool read_;
    };
    
    return std::make_shared<SimpleMessageStream>(message);
}

// BuildRequestJSON - helper to build OpenAI API request
nlohmann::json OpenAIChatModel::BuildRequestJSON(
    const std::vector<schema::Message>& messages,
    const std::vector<compose::Option>& opts) {
    
    nlohmann::json request;
    
    // Model name
    request["model"] = config_->model;
    
    // Messages
    request["messages"] = nlohmann::json::array();
    for (const auto& msg : messages) {
        nlohmann::json msg_json;
        msg_json["role"] = schema::RoleToString(msg.role);
        msg_json["content"] = msg.content;
        
        // Add tool calls if present
        if (!msg.tool_calls.empty()) {
            msg_json["tool_calls"] = nlohmann::json::array();
            for (const auto& tc : msg.tool_calls) {
                nlohmann::json tc_json;
                tc_json["id"] = tc.id;
                tc_json["type"] = tc.type;
                tc_json["function"] = {
                    {"name", tc.function.name},
                    {"arguments", tc.function.arguments}
                };
                msg_json["tool_calls"].push_back(tc_json);
            }
        }
        
        // Add tool call ID if this is a tool result
        if (!msg.tool_call_id.empty()) {
            msg_json["tool_call_id"] = msg.tool_call_id;
        }
        
        request["messages"].push_back(msg_json);
    }
    
    // Generation parameters
    request["temperature"] = config_->temperature;
    if (config_->max_tokens > 0) {
        request["max_tokens"] = config_->max_tokens;
    }
    request["top_p"] = config_->top_p;
    
    if (!config_->stop.empty()) {
        request["stop"] = config_->stop;
    }
    
    // Tools (if bound)
    if (!config_->tools_json.empty()) {
        request["tools"] = config_->tools_json;
        request["tool_choice"] = ConvertToolChoiceToOpenAIString(config_->tool_choice);
    }
    
    return request;
}

// ParseResponseJSON - helper to parse OpenAI API response
schema::Message OpenAIChatModel::ParseResponseJSON(const nlohmann::json& response) {
    schema::Message result;
    
    // Extract message from response
    // OpenAI response format:
    // {
    //   "choices": [
    //     {
    //       "message": {
    //         "role": "assistant",
    //         "content": "...",
    //         "tool_calls": [...]
    //       }
    //     }
    //   ]
    // }
    
    if (response.contains("choices") && !response["choices"].empty()) {
        const auto& choice = response["choices"][0];
        const auto& message = choice["message"];
        
        // Role
        if (message.contains("role")) {
            result.role = schema::StringToRole(message["role"].get<std::string>());
        }
        
        // Content
        if (message.contains("content") && !message["content"].is_null()) {
            result.content = message["content"].get<std::string>();
        }
        
        // Tool calls
        if (message.contains("tool_calls")) {
            for (const auto& tc_json : message["tool_calls"]) {
                schema::ToolCall tc;
                tc.id = tc_json["id"].get<std::string>();
                tc.type = tc_json["type"].get<std::string>();
                tc.function.name = tc_json["function"]["name"].get<std::string>();
                tc.function.arguments = tc_json["function"]["arguments"].get<std::string>();
                
                result.tool_calls.push_back(tc);
            }
        }
    }
    
    return result;
}

// MakeAPIRequest - helper to make HTTP request to OpenAI API
nlohmann::json OpenAIChatModel::MakeAPIRequest(
    const std::string& endpoint,
    const nlohmann::json& request_json) {
    
    // TODO: Implement actual HTTP request using libcurl or similar
    // This is a placeholder
    
    // In real implementation:
    // 1. Build HTTP POST request
    // 2. Set headers (Authorization: Bearer <api_key>, Content-Type: application/json)
    // 3. Send request to base_url + endpoint
    // 4. Parse JSON response
    // 5. Handle errors (rate limits, timeouts, etc.)
    // 6. Implement retries with exponential backoff
    
    throw std::runtime_error("OpenAIChatModel: HTTP client not implemented yet");
}

} // namespace components
} // namespace eino
