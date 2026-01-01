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

#ifndef EINO_CPP_COMPONENTS_MODEL_WITH_TOOLS_H_
#define EINO_CPP_COMPONENTS_MODEL_WITH_TOOLS_H_

#include "model.h"
#include "../schema/tool.h"
#include <vector>
#include <memory>
#include <stdexcept>

namespace eino {
namespace components {

// ToolCallingChatModelBase provides a base implementation for ToolCallingChatModel
// Derived classes should implement CloneWithTools() to create a new instance
// Aligns with eino/components/model/interface.go:50-56 (ToolCallingChatModel)
//
// Usage:
//   class MyModel : public ToolCallingChatModelBase<MyModel> {
//   protected:
//       std::shared_ptr<MyModel> CloneWithToolsImpl(
//           const std::vector<schema::ToolInfo>& tools) override {
//           // Deep copy config
//           auto new_config = std::make_shared<Config>(*config_);
//           // Set tools
//           new_config->tools = tools;
//           new_config->tools_json = ConvertToolsToJSON(tools);
//           // Return new instance
//           return std::make_shared<MyModel>(ctx_, new_config);
//       }
//   };
template<typename Derived>
class ToolCallingChatModelBase : public ToolCallingChatModel {
public:
    virtual ~ToolCallingChatModelBase() = default;
    
    // WithTools returns a new instance with tools bound
    // Aligns with eino/components/model/interface.go:55
    // This method implements the immutable pattern:
    // 1. Deep copy configuration
    // 2. Bind tools to new config
    // 3. Return new instance (does NOT modify current instance)
    std::shared_ptr<ToolCallingChatModel> WithTools(
        const std::vector<schema::ToolInfo>& tools) override {
        
        if (tools.empty()) {
            // Return a copy even with empty tools to maintain immutability
            return CloneWithToolsImpl(tools);
        }
        
        // Delegate to derived class for actual cloning
        return CloneWithToolsImpl(tools);
    }
    
protected:
    // Derived classes must implement this to create a new instance with tools
    // This ensures the implementation-specific details (config copy, etc.)
    // are handled by the concrete model class
    virtual std::shared_ptr<Derived> CloneWithToolsImpl(
        const std::vector<schema::ToolInfo>& tools) = 0;
};

// ConvertToolsToJSON converts ToolInfo vector to JSON format for model APIs
// Aligns with tool description generation in various model implementations
// Returns JSON array in OpenAI tools format:
// [
//   {
//     "type": "function",
//     "function": {
//       "name": "tool_name",
//       "description": "tool_desc",
//       "parameters": { ... }
//     }
//   }
// ]
inline nlohmann::json ConvertToolsToJSON(
    const std::vector<schema::ToolInfo>& tools) {
    
    nlohmann::json tools_json = nlohmann::json::array();
    
    for (const auto& tool : tools) {
        nlohmann::json tool_json;
        tool_json["type"] = "function";
        
        nlohmann::json func_json;
        func_json["name"] = tool.name;
        func_json["description"] = tool.desc;
        
        // Convert parameters to JSONSchema
        if (tool.params) {
            func_json["parameters"] = tool.params->ToJSONSchema();
        } else {
            // No parameters - empty object schema
            func_json["parameters"] = {
                {"type", "object"},
                {"properties", nlohmann::json::object()},
                {"required", nlohmann::json::array()}
            };
        }
        
        tool_json["function"] = func_json;
        tools_json.push_back(tool_json);
    }
    
    return tools_json;
}

// ConvertToolsToAnthropicFormat converts ToolInfo to Anthropic's tool format
// Anthropic uses a different format than OpenAI
// [
//   {
//     "name": "tool_name",
//     "description": "tool_desc",
//     "input_schema": { ... }
//   }
// ]
inline nlohmann::json ConvertToolsToAnthropicFormat(
    const std::vector<schema::ToolInfo>& tools) {
    
    nlohmann::json tools_json = nlohmann::json::array();
    
    for (const auto& tool : tools) {
        nlohmann::json tool_json;
        tool_json["name"] = tool.name;
        tool_json["description"] = tool.desc;
        
        // Anthropic uses "input_schema" instead of "parameters"
        if (tool.params) {
            tool_json["input_schema"] = tool.params->ToJSONSchema();
        } else {
            tool_json["input_schema"] = {
                {"type", "object"},
                {"properties", nlohmann::json::object()},
                {"required", nlohmann::json::array()}
            };
        }
        
        tools_json.push_back(tool_json);
    }
    
    return tools_json;
}

// ConvertToolChoiceToString converts ToolChoice enum to string for API calls
// Different providers use different terminology
inline std::string ConvertToolChoiceToOpenAIString(schema::ToolChoice choice) {
    switch (choice) {
        case schema::ToolChoice::kForbidden:
            return "none";
        case schema::ToolChoice::kAllowed:
            return "auto";
        case schema::ToolChoice::kForced:
            return "required";
        default:
            return "auto";
    }
}

inline std::string ConvertToolChoiceToAnthropicString(schema::ToolChoice choice) {
    switch (choice) {
        case schema::ToolChoice::kForbidden:
            return "none";  // Not officially supported by Anthropic
        case schema::ToolChoice::kAllowed:
            return "auto";
        case schema::ToolChoice::kForced:
            return "any";
        default:
            return "auto";
    }
}

} // namespace components
} // namespace eino

#endif // EINO_CPP_COMPONENTS_MODEL_WITH_TOOLS_H_
