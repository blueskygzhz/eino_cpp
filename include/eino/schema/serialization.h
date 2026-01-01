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

#ifndef EINO_CPP_SCHEMA_SERIALIZATION_H_
#define EINO_CPP_SCHEMA_SERIALIZATION_H_

#include "types.h"
#include "../internal/serialization.h"
#include <nlohmann/json.hpp>

namespace eino {
namespace schema {

// ============================================================================
// Message Serialization - Aligns with eino schema.Message
// ============================================================================

/**
 * Serialize Message to JSON
 * Aligns with Go's JSON marshaling of schema.Message
 */
nlohmann::json to_json(const Message& msg);

/**
 * Deserialize Message from JSON  
 * Aligns with Go's JSON unmarshaling of schema.Message
 */
Message from_json_message(const nlohmann::json& j);

// ============================================================================
// Document Serialization - Aligns with eino schema.Document
// ============================================================================

/**
 * Serialize Document to JSON
 */
nlohmann::json to_json(const Document& doc);

/**
 * Deserialize Document from JSON
 */
Document from_json_document(const nlohmann::json& j);

// ============================================================================
// ToolCall Serialization - Aligns with eino schema.ToolCall
// ============================================================================

/**
 * Serialize ToolCall to JSON
 */
nlohmann::json to_json(const ToolCall& tc);

/**
 * Deserialize ToolCall from JSON
 */
ToolCall from_json_toolcall(const nlohmann::json& j);

// ============================================================================
// ToolResponse Serialization
// ============================================================================

/**
 * Serialize ToolResponse to JSON
 */
nlohmann::json to_json(const ToolResponse& tr);

/**
 * Deserialize ToolResponse from JSON
 */
ToolResponse from_json_toolresponse(const nlohmann::json& j);

// ============================================================================
// ChatModelRequest/Response Serialization
// ============================================================================

/**
 * Serialize ChatModelRequest to JSON
 */
nlohmann::json to_json(const ChatModelRequest& req);

/**
 * Deserialize ChatModelRequest from JSON
 */
ChatModelRequest from_json_chatmodelrequest(const nlohmann::json& j);

/**
 * Serialize ChatModelResponse to JSON
 */
nlohmann::json to_json(const ChatModelResponse& resp);

/**
 * Deserialize ChatModelResponse from JSON
 */
ChatModelResponse from_json_chatmodelresponse(const nlohmann::json& j);

// ============================================================================
// Registration Helper
// ============================================================================

/**
 * RegisterSchemaTypes registers all schema types with the global TypeRegistry
 * Call this once at program initialization
 * Aligns with eino schema.init() or explicit registration
 */
void RegisterSchemaTypes();

}  // namespace schema
}  // namespace eino

// ============================================================================
// nlohmann::json Integration (ADL)
// ============================================================================

namespace nlohmann {

// Message
template<>
struct adl_serializer<eino::schema::Message> {
    static void to_json(json& j, const eino::schema::Message& msg) {
        j = eino::schema::to_json(msg);
    }
    
    static void from_json(const json& j, eino::schema::Message& msg) {
        msg = eino::schema::from_json_message(j);
    }
};

// Document
template<>
struct adl_serializer<eino::schema::Document> {
    static void to_json(json& j, const eino::schema::Document& doc) {
        j = eino::schema::to_json(doc);
    }
    
    static void from_json(const json& j, eino::schema::Document& doc) {
        doc = eino::schema::from_json_document(j);
    }
};

// ToolCall
template<>
struct adl_serializer<eino::schema::ToolCall> {
    static void to_json(json& j, const eino::schema::ToolCall& tc) {
        j = eino::schema::to_json(tc);
    }
    
    static void from_json(const json& j, eino::schema::ToolCall& tc) {
        tc = eino::schema::from_json_toolcall(j);
    }
};

// ToolResponse
template<>
struct adl_serializer<eino::schema::ToolResponse> {
    static void to_json(json& j, const eino::schema::ToolResponse& tr) {
        j = eino::schema::to_json(tr);
    }
    
    static void from_json(const json& j, eino::schema::ToolResponse& tr) {
        tr = eino::schema::from_json_toolresponse(j);
    }
};

}  // namespace nlohmann

#endif  // EINO_CPP_SCHEMA_SERIALIZATION_H_
