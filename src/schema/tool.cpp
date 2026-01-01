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

#include "eino/schema/tool.h"

namespace eino {
namespace schema {

// DataTypeToString converts DataType enum to string
// Aligns with eino/schema/tool.go:22-30 (DataType constants)
std::string DataTypeToString(DataType type) {
    switch (type) {
        case DataType::kObject:  return "object";
        case DataType::kNumber:  return "number";
        case DataType::kInteger: return "integer";
        case DataType::kString:  return "string";
        case DataType::kArray:   return "array";
        case DataType::kNull:    return "null";
        case DataType::kBoolean: return "boolean";
        default:                 return "string";
    }
}

// ToolChoiceToString converts ToolChoice enum to string
// Aligns with eino/schema/tool.go:35-50 (ToolChoice constants)
std::string ToolChoiceToString(ToolChoice choice) {
    switch (choice) {
        case ToolChoice::kForbidden: return "forbidden";
        case ToolChoice::kAllowed:   return "allowed";
        case ToolChoice::kForced:    return "forced";
        default:                     return "allowed";
    }
}

// ParamsOneOf::FromParams creates ParamsOneOf from ParameterInfo map
// Aligns with eino/schema/tool.go:98-102 (NewParamsOneOfByParams)
std::shared_ptr<ParamsOneOf> ParamsOneOf::FromParams(
    const std::map<std::string, std::shared_ptr<ParameterInfo>>& params) {
    
    auto result = std::make_shared<ParamsOneOf>();
    result->params = params;
    return result;
}

// ParamsOneOf::FromJSONSchema creates ParamsOneOf from JSONSchema
// Aligns with eino/schema/tool.go:104-108 (NewParamsOneOfByJSONSchema)
std::shared_ptr<ParamsOneOf> ParamsOneOf::FromJSONSchema(const json& schema) {
    auto result = std::make_shared<ParamsOneOf>();
    result->jsonschema = schema;
    return result;
}

// ParameterInfoToJSONSchema converts ParameterInfo to JSONSchema format
// Aligns with eino/schema/tool.go:134-170 (paramInfoToJSONSchema)
json ParameterInfoToJSONSchema(const std::shared_ptr<ParameterInfo>& param_info) {
    if (!param_info) {
        return json::object();
    }
    
    json js;
    js["type"] = DataTypeToString(param_info->type);
    
    if (!param_info->desc.empty()) {
        js["description"] = param_info->desc;
    }
    
    // Handle enum values (for string type)
    if (!param_info->enum_values.empty()) {
        js["enum"] = json::array();
        for (const auto& enum_val : param_info->enum_values) {
            js["enum"].push_back(enum_val);
        }
    }
    
    // Handle array element type
    if (param_info->elem_info) {
        js["items"] = ParameterInfoToJSONSchema(param_info->elem_info);
    }
    
    // Handle object sub-parameters
    if (!param_info->sub_params.empty()) {
        js["properties"] = json::object();
        js["required"] = json::array();
        
        for (const auto& [key, sub_param] : param_info->sub_params) {
            js["properties"][key] = ParameterInfoToJSONSchema(sub_param);
            
            if (sub_param && sub_param->required) {
                js["required"].push_back(key);
            }
        }
    }
    
    return js;
}

// ParamsOneOf::ToJSONSchema converts ParamsOneOf to JSONSchema
// Aligns with eino/schema/tool.go:110-132 (ToJSONSchema)
json ParamsOneOf::ToJSONSchema() const {
    // If already JSONSchema, return it directly
    if (!jsonschema.is_null()) {
        return jsonschema;
    }
    
    // Convert from ParameterInfo map
    if (!params.empty()) {
        json schema;
        schema["type"] = "object";
        schema["properties"] = json::object();
        schema["required"] = json::array();
        
        for (const auto& [key, param_info] : params) {
            schema["properties"][key] = ParameterInfoToJSONSchema(param_info);
            
            if (param_info && param_info->required) {
                schema["required"].push_back(key);
            }
        }
        
        return schema;
    }
    
    // Empty params - return minimal object schema
    return json{
        {"type", "object"},
        {"properties", json::object()},
        {"required", json::array()}
    };
}

} // namespace schema
} // namespace eino
