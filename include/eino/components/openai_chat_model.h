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

#ifndef EINO_CPP_COMPONENTS_OPENAI_CHAT_MODEL_H_
#define EINO_CPP_COMPONENTS_OPENAI_CHAT_MODEL_H_

#include "model_with_tools.h"
#include "../schema/message.h"
#include "../schema/tool.h"
#include <string>
#include <memory>
#include <nlohmann/json.hpp>

namespace eino {
namespace components {

// OpenAIChatModelConfig for OpenAI Chat Completion API
// Aligns with OpenAI model configuration pattern
struct OpenAIChatModelConfig {
    std::string api_key;                    // OpenAI API key
    std::string model = "gpt-4";            // Model name
    std::string base_url = "https://api.openai.com/v1";  // API base URL
    
    // Generation parameters
    float temperature = 1.0f;               // Sampling temperature (0-2)
    int max_tokens = -1;                    // Max tokens to generate (-1 = no limit)
    float top_p = 1.0f;                     // Nucleus sampling (0-1)
    std::vector<std::string> stop;          // Stop sequences
    
    // Tool calling parameters
    std::vector<schema::ToolInfo> tools;    // Bound tools
    nlohmann::json tools_json;              // Tools in OpenAI JSON format
    schema::ToolChoice tool_choice = schema::ToolChoice::kAllowed;  // Tool choice strategy
    
    // Request options
    int timeout_seconds = 60;               // Request timeout
    int max_retries = 3;                    // Max retry attempts
    
    OpenAIChatModelConfig() = default;
    
    // Deep copy constructor
    OpenAIChatModelConfig(const OpenAIChatModelConfig& other)
        : api_key(other.api_key),
          model(other.model),
          base_url(other.base_url),
          temperature(other.temperature),
          max_tokens(other.max_tokens),
          top_p(other.top_p),
          stop(other.stop),
          tools(other.tools),
          tools_json(other.tools_json),
          tool_choice(other.tool_choice),
          timeout_seconds(other.timeout_seconds),
          max_retries(other.max_retries) {}
};

// OpenAIChatModel implements ToolCallingChatModel for OpenAI Chat Completion API
// Demonstrates the WithTools() immutable pattern
// Aligns with eino ToolCallingChatModel interface
//
// Example usage:
//   auto config = std::make_shared<OpenAIChatModelConfig>();
//   config->api_key = "sk-...";
//   config->model = "gpt-4o";
//   
//   auto model = std::make_shared<OpenAIChatModel>(ctx, config);
//   
//   // Bind tools (returns NEW instance, does not modify original)
//   std::vector<schema::ToolInfo> tools = {calculator_tool, search_tool};
//   auto model_with_tools = model->WithTools(tools);
//   
//   // Original model is unchanged, can be reused
//   auto result1 = model->Generate(ctx, messages);  // No tools
//   auto result2 = model_with_tools->Generate(ctx, messages);  // With tools
class OpenAIChatModel : public ToolCallingChatModelBase<OpenAIChatModel> {
public:
    // Constructor
    // ctx: context for logging, tracing, etc.
    // config: model configuration (will be stored as shared_ptr)
    OpenAIChatModel(
        std::shared_ptr<compose::Context> ctx,
        std::shared_ptr<OpenAIChatModelConfig> config);
    
    virtual ~OpenAIChatModel() = default;
    
    // Generate a single message response
    // Aligns with eino/components/model/interface.go:31-32 (Generate)
    schema::Message Generate(
        std::shared_ptr<compose::Context> ctx,
        const std::vector<schema::Message>& input,
        const std::vector<compose::Option>& opts = {}) override;
    
    // Stream generates a streaming response
    // Aligns with eino/components/model/interface.go:33-34 (Stream)
    std::shared_ptr<schema::StreamReader<schema::Message>> Stream(
        std::shared_ptr<compose::Context> ctx,
        const std::vector<schema::Message>& input,
        const std::vector<compose::Option>& opts = {}) override;
    
    // Invoke - implements Runnable interface
    schema::Message Invoke(
        std::shared_ptr<compose::Context> ctx,
        const std::vector<schema::Message>& input,
        const std::vector<compose::Option>& opts = {}) override {
        return Generate(ctx, input, opts);
    }
    
    // GetInputType - implements Runnable interface
    const std::type_info& GetInputType() const override {
        return typeid(std::vector<schema::Message>);
    }
    
    // GetOutputType - implements Runnable interface
    const std::type_info& GetOutputType() const override {
        return typeid(schema::Message);
    }
    
    // GetComponentType - implements Runnable interface
    std::string GetComponentType() const override {
        return "OpenAIChatModel";
    }
    
    // Get current configuration (read-only)
    std::shared_ptr<const OpenAIChatModelConfig> GetConfig() const {
        return config_;
    }
    
protected:
    // CloneWithToolsImpl creates a new instance with tools bound
    // Aligns with eino/components/model/interface.go:55 (WithTools)
    // Implementation pattern:
    // 1. Deep copy config
    // 2. Set tools in new config
    // 3. Convert tools to JSON format
    // 4. Return new instance
    std::shared_ptr<OpenAIChatModel> CloneWithToolsImpl(
        const std::vector<schema::ToolInfo>& tools) override;
    
private:
    std::shared_ptr<compose::Context> ctx_;
    std::shared_ptr<OpenAIChatModelConfig> config_;
    
    // Helper: Build request JSON for OpenAI API
    nlohmann::json BuildRequestJSON(
        const std::vector<schema::Message>& messages,
        const std::vector<compose::Option>& opts);
    
    // Helper: Parse response JSON to Message
    schema::Message ParseResponseJSON(const nlohmann::json& response);
    
    // Helper: Make HTTP request to OpenAI API
    nlohmann::json MakeAPIRequest(
        const std::string& endpoint,
        const nlohmann::json& request_json);
};

// NewOpenAIChatModel creates a new OpenAI chat model instance
// Convenience factory function
inline std::shared_ptr<OpenAIChatModel> NewOpenAIChatModel(
    std::shared_ptr<compose::Context> ctx,
    std::shared_ptr<OpenAIChatModelConfig> config) {
    
    if (!config) {
        throw std::invalid_argument("Config cannot be null");
    }
    
    if (config->api_key.empty()) {
        throw std::invalid_argument("API key is required");
    }
    
    return std::make_shared<OpenAIChatModel>(ctx, config);
}

} // namespace components
} // namespace eino

#endif // EINO_CPP_COMPONENTS_OPENAI_CHAT_MODEL_H_
