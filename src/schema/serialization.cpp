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

#include "eino/schema/serialization.h"

namespace eino {
namespace schema {

// ============================================================================
// Helper function to convert RoleType enum
// ============================================================================

static std::string RoleTypeToString(RoleType role) {
    switch (role) {
        case RoleType::User: return "user";
        case RoleType::Assistant: return "assistant";
        case RoleType::System: return "system";
        case RoleType::Tool: return "tool";
        case RoleType::Function: return "function";
        case RoleType::Generic: return "generic";
        default: return "user";
    }
}

static RoleType StringToRoleType(const std::string& s) {
    if (s == "user") return RoleType::User;
    if (s == "assistant") return RoleType::Assistant;
    if (s == "system") return RoleType::System;
    if (s == "tool") return RoleType::Tool;
    if (s == "function") return RoleType::Function;
    if (s == "generic") return RoleType::Generic;
    return RoleType::User;  // default
}

// ============================================================================
// Message Serialization
// ============================================================================

nlohmann::json to_json(const Message& msg) {
    nlohmann::json j;
    
    j["role"] = RoleTypeToString(msg.role);
    j["content"] = msg.content;
    
    if (!msg.name.empty()) {
        j["name"] = msg.name;
    }
    
    if (!msg.tool_call_id.empty()) {
        j["tool_call_id"] = msg.tool_call_id;
    }
    
    if (!msg.tool_calls.empty()) {
        nlohmann::json tool_calls_array = nlohmann::json::array();
        for (const auto& tc : msg.tool_calls) {
            tool_calls_array.push_back(to_json(tc));
        }
        j["tool_calls"] = tool_calls_array;
    }
    
    if (!msg.multi_content.empty()) {
        // Serialize multi_content if needed
        j["multi_content"] = msg.multi_content;
    }
    
    if (!msg.response_metadata.empty()) {
        j["response_metadata"] = msg.response_metadata;
    }
    
    if (!msg.extra.empty()) {
        j["extra"] = msg.extra;
    }
    
    return j;
}

Message from_json_message(const nlohmann::json& j) {
    Message msg;
    
    if (j.contains("role")) {
        msg.role = StringToRoleType(j["role"].get<std::string>());
    }
    
    if (j.contains("content")) {
        msg.content = j["content"].get<std::string>();
    }
    
    if (j.contains("name")) {
        msg.name = j["name"].get<std::string>();
    }
    
    if (j.contains("tool_call_id")) {
        msg.tool_call_id = j["tool_call_id"].get<std::string>();
    }
    
    if (j.contains("tool_calls")) {
        for (const auto& tc_json : j["tool_calls"]) {
            msg.tool_calls.push_back(from_json_toolcall(tc_json));
        }
    }
    
    if (j.contains("multi_content")) {
        msg.multi_content = j["multi_content"];
    }
    
    if (j.contains("response_metadata")) {
        msg.response_metadata = j["response_metadata"];
    }
    
    if (j.contains("extra")) {
        msg.extra = j["extra"];
    }
    
    return msg;
}

// ============================================================================
// Document Serialization
// ============================================================================

nlohmann::json to_json(const Document& doc) {
    nlohmann::json j;
    
    j["content"] = doc.content;
    
    if (!doc.metadata.empty()) {
        j["metadata"] = doc.metadata;
    }
    
    if (doc.vector && !doc.vector->empty()) {
        j["vector"] = *doc.vector;
    }
    
    return j;
}

Document from_json_document(const nlohmann::json& j) {
    Document doc;
    
    if (j.contains("content")) {
        doc.content = j["content"].get<std::string>();
    }
    
    if (j.contains("metadata")) {
        doc.metadata = j["metadata"];
    }
    
    if (j.contains("vector")) {
        doc.vector = std::make_shared<std::vector<float>>(
            j["vector"].get<std::vector<float>>());
    }
    
    return doc;
}

// ============================================================================
// ToolCall Serialization
// ============================================================================

nlohmann::json to_json(const ToolCall& tc) {
    nlohmann::json j;
    
    if (!tc.id.empty()) {
        j["id"] = tc.id;
    }
    
    if (tc.index.has_value()) {
        j["index"] = tc.index.value();
    }
    
    j["function"] = {
        {"name", tc.function.name},
        {"arguments", tc.function.arguments}
    };
    
    j["type"] = tc.type;
    
    return j;
}

ToolCall from_json_toolcall(const nlohmann::json& j) {
    ToolCall tc;
    
    if (j.contains("id")) {
        tc.id = j["id"].get<std::string>();
    }
    
    if (j.contains("index")) {
        tc.index = j["index"].get<int>();
    }
    
    if (j.contains("function")) {
        const auto& func = j["function"];
        if (func.contains("name")) {
            tc.function.name = func["name"].get<std::string>();
        }
        if (func.contains("arguments")) {
            tc.function.arguments = func["arguments"].get<std::string>();
        }
    }
    
    if (j.contains("type")) {
        tc.type = j["type"].get<std::string>();
    }
    
    return tc;
}

// ============================================================================
// ToolResponse Serialization
// ============================================================================

nlohmann::json to_json(const ToolResponse& tr) {
    nlohmann::json j;
    
    j["tool_call_id"] = tr.tool_call_id;
    j["content"] = tr.content;
    
    if (!tr.name.empty()) {
        j["name"] = tr.name;
    }
    
    return j;
}

ToolResponse from_json_toolresponse(const nlohmann::json& j) {
    ToolResponse tr;
    
    if (j.contains("tool_call_id")) {
        tr.tool_call_id = j["tool_call_id"].get<std::string>();
    }
    
    if (j.contains("content")) {
        tr.content = j["content"].get<std::string>();
    }
    
    if (j.contains("name")) {
        tr.name = j["name"].get<std::string>();
    }
    
    return tr;
}

// ============================================================================
// ChatModelRequest Serialization
// ============================================================================

nlohmann::json to_json(const ChatModelRequest& req) {
    nlohmann::json j;
    
    // Serialize messages
    nlohmann::json messages_array = nlohmann::json::array();
    for (const auto& msg : req.messages) {
        messages_array.push_back(to_json(msg));
    }
    j["messages"] = messages_array;
    
    // Config
    if (!req.config.empty()) {
        j["config"] = req.config;
    }
    
    return j;
}

ChatModelRequest from_json_chatmodelrequest(const nlohmann::json& j) {
    ChatModelRequest req;
    
    if (j.contains("messages")) {
        for (const auto& msg_json : j["messages"]) {
            req.messages.push_back(from_json_message(msg_json));
        }
    }
    
    if (j.contains("config")) {
        req.config = j["config"];
    }
    
    return req;
}

// ============================================================================
// ChatModelResponse Serialization
// ============================================================================

nlohmann::json to_json(const ChatModelResponse& resp) {
    nlohmann::json j;
    
    j["message"] = to_json(resp.message);
    
    // Usage
    j["usage"] = {
        {"prompt_tokens", resp.usage.prompt_tokens},
        {"completion_tokens", resp.usage.completion_tokens},
        {"total_tokens", resp.usage.total_tokens}
    };
    
    return j;
}

ChatModelResponse from_json_chatmodelresponse(const nlohmann::json& j) {
    ChatModelResponse resp;
    
    if (j.contains("message")) {
        resp.message = from_json_message(j["message"]);
    }
    
    if (j.contains("usage")) {
        const auto& usage = j["usage"];
        if (usage.contains("prompt_tokens")) {
            resp.usage.prompt_tokens = usage["prompt_tokens"].get<int>();
        }
        if (usage.contains("completion_tokens")) {
            resp.usage.completion_tokens = usage["completion_tokens"].get<int>();
        }
        if (usage.contains("total_tokens")) {
            resp.usage.total_tokens = usage["total_tokens"].get<int>();
        }
    }
    
    return resp;
}

// ============================================================================
// Type Registration
// ============================================================================

void RegisterSchemaTypes() {
    auto& registry = eino::internal::TypeRegistry::Instance();
    
    // Register schema types
    registry.Register<Message>("eino.schema.Message");
    registry.Register<Document>("eino.schema.Document");
    registry.Register<ToolCall>("eino.schema.ToolCall");
    registry.Register<ToolResponse>("eino.schema.ToolResponse");
    registry.Register<ChatModelRequest>("eino.schema.ChatModelRequest");
    registry.Register<ChatModelResponse>("eino.schema.ChatModelResponse");
    
    // Register vector types commonly used
    registry.Register<std::vector<Message>>("eino.schema.MessageArray");
    registry.Register<std::vector<Document>>("eino.schema.DocumentArray");
}

}  // namespace schema
}  // namespace eino

// ============================================================================
// Specialized SerializeValue/DeserializeValue for schema types
// ============================================================================

namespace eino {
namespace internal {

// Message
template<>
nlohmann::json SerializeValue(const schema::Message& msg) {
    return schema::to_json(msg);
}

template<>
schema::Message DeserializeValue(const nlohmann::json& j) {
    return schema::from_json_message(j);
}

// Document
template<>
nlohmann::json SerializeValue(const schema::Document& doc) {
    return schema::to_json(doc);
}

template<>
schema::Document DeserializeValue(const nlohmann::json& j) {
    return schema::from_json_document(j);
}

// ToolCall
template<>
nlohmann::json SerializeValue(const schema::ToolCall& tc) {
    return schema::to_json(tc);
}

template<>
schema::ToolCall DeserializeValue(const nlohmann::json& j) {
    return schema::from_json_toolcall(j);
}

// ToolResponse
template<>
nlohmann::json SerializeValue(const schema::ToolResponse& tr) {
    return schema::to_json(tr);
}

template<>
schema::ToolResponse DeserializeValue(const nlohmann::json& j) {
    return schema::from_json_toolresponse(j);
}

// ChatModelRequest
template<>
nlohmann::json SerializeValue(const schema::ChatModelRequest& req) {
    return schema::to_json(req);
}

template<>
schema::ChatModelRequest DeserializeValue(const nlohmann::json& j) {
    return schema::from_json_chatmodelrequest(j);
}

// ChatModelResponse
template<>
nlohmann::json SerializeValue(const schema::ChatModelResponse& resp) {
    return schema::to_json(resp);
}

template<>
schema::ChatModelResponse DeserializeValue(const nlohmann::json& j) {
    return schema::from_json_chatmodelresponse(j);
}

}  // namespace internal
}  // namespace eino
