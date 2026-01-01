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

#ifndef EINO_CPP_SCHEMA_TOOL_H_
#define EINO_CPP_SCHEMA_TOOL_H_

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <nlohmann/json.hpp>

namespace eino {
namespace schema {

using json = nlohmann::json;

// DataType for tool parameters
enum class DataType {
    kObject,
    kNumber,
    kInteger,
    kString,
    kArray,
    kNull,
    kBoolean
};

// ToolChoice controls how the model calls tools
enum class ToolChoice {
    kForbidden,  // Model should not call any tools (OpenAI: "none")
    kAllowed,    // Model can choose to call tools or not (OpenAI: "auto")
    kForced      // Model must call one or more tools (OpenAI: "required")
};

// ParameterInfo describes a tool parameter
struct ParameterInfo {
    DataType type;                                  // Type of parameter
    std::shared_ptr<ParameterInfo> elem_info;       // Element type (for arrays)
    std::map<std::string, std::shared_ptr<ParameterInfo>> sub_params;  // Sub-parameters (for objects)
    std::string desc;                               // Description
    std::vector<std::string> enum_values;           // Enum values (for strings)
    bool required = false;                          // Whether required
    
    ParameterInfo() = default;
    ParameterInfo(DataType t, const std::string& description = "", bool req = false)
        : type(t), desc(description), required(req) {}
};

// ParamsOneOf represents tool parameters (can be ParameterInfo map or JSONSchema)
struct ParamsOneOf {
    std::map<std::string, std::shared_ptr<ParameterInfo>> params;
    json jsonschema;  // JSONSchema representation
    
    ParamsOneOf() = default;
    
    // Create from ParameterInfo map
    static std::shared_ptr<ParamsOneOf> FromParams(
        const std::map<std::string, std::shared_ptr<ParameterInfo>>& params);
    
    // Create from JSONSchema
    static std::shared_ptr<ParamsOneOf> FromJSONSchema(const json& schema);
    
    // Convert to JSONSchema
    json ToJSONSchema() const;
};

// ToolInfo is the information of a tool
struct ToolInfo {
    std::string name;                      // Unique name
    std::string desc;                      // Description
    std::map<std::string, json> extra;     // Extra information
    std::shared_ptr<ParamsOneOf> params;   // Parameters description
    
    ToolInfo() = default;
    ToolInfo(const std::string& n, const std::string& d)
        : name(n), desc(d) {}
};

// Helper function to convert ParameterInfo to JSONSchema
json ParameterInfoToJSONSchema(const std::shared_ptr<ParameterInfo>& param_info);

// Helper function to convert DataType to string
std::string DataTypeToString(DataType type);

// Helper function to convert ToolChoice to string
std::string ToolChoiceToString(ToolChoice choice);

} // namespace schema
} // namespace eino

#endif // EINO_CPP_SCHEMA_TOOL_H_
