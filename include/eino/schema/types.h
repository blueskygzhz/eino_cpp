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

#ifndef EINO_CPP_SCHEMA_TYPES_H_
#define EINO_CPP_SCHEMA_TYPES_H_

#include <string>
#include <map>
#include <vector>
#include <memory>
#include <nlohmann/json.hpp>

namespace eino {
namespace schema {

using json = nlohmann::json;

// ============================================================================
// Enumerations and Basic Types
// ============================================================================

// FormatType used by MessageTemplate.Format
enum class FormatType {
    kFString = 0,  // Python f-string format
    kGoTemplate = 1,  // Go template format
    kJinja2 = 2,      // Jinja2 format
};

// RoleType is the type of the role of a message
enum class RoleType {
    kAssistant,  // Message returned by ChatModel
    kUser,       // User message
    kSystem,     // System message
    kTool,       // Tool call output
};

// Convert RoleType to string
inline std::string RoleTypeToString(RoleType role) {
    switch (role) {
        case RoleType::kAssistant: return "assistant";
        case RoleType::kUser: return "user";
        case RoleType::kSystem: return "system";
        case RoleType::kTool: return "tool";
        default: return "unknown";
    }
}

// ChatMessagePartType is the type of the part in a chat message
enum class ChatMessagePartType {
    kText,      // "text"
    kImageURL,  // "image_url"
    kAudioURL,  // "audio_url"
    kVideoURL,  // "video_url"
    kFileURL,   // "file_url"
};

// Convert ChatMessagePartType to string
inline std::string ChatMessagePartTypeToString(ChatMessagePartType type) {
    switch (type) {
        case ChatMessagePartType::kText: return "text";
        case ChatMessagePartType::kImageURL: return "image_url";
        case ChatMessagePartType::kAudioURL: return "audio_url";
        case ChatMessagePartType::kVideoURL: return "video_url";
        case ChatMessagePartType::kFileURL: return "file_url";
        default: return "unknown";
    }
}

// ImageURLDetail is the detail level of an image URL
enum class ImageURLDetail {
    kHigh,   // "high"
    kLow,    // "low"
    kAuto,   // "auto"
};

// Convert ImageURLDetail to string
inline std::string ImageURLDetailToString(ImageURLDetail detail) {
    switch (detail) {
        case ImageURLDetail::kHigh: return "high";
        case ImageURLDetail::kLow: return "low";
        case ImageURLDetail::kAuto: return "auto";
        default: return "auto";
    }
}

// DataType for tool parameters
enum class DataType {
    kObject,    // "object"
    kNumber,    // "number"
    kInteger,   // "integer"
    kString,    // "string"
    kArray,     // "array"
    kNull,      // "null"
    kBoolean,   // "boolean"
};

// Convert DataType to string
inline std::string DataTypeToString(DataType type) {
    switch (type) {
        case DataType::kObject: return "object";
        case DataType::kNumber: return "number";
        case DataType::kInteger: return "integer";
        case DataType::kString: return "string";
        case DataType::kArray: return "array";
        case DataType::kNull: return "null";
        case DataType::kBoolean: return "boolean";
        default: return "unknown";
    }
}

// ToolChoice controls how the model calls tools
enum class ToolChoice {
    kForbidden,  // "forbidden" - model should not call any tools
    kAllowed,    // "allowed" - model can choose to call tools or not
    kForced,     // "forced" - model must call one or more tools
};

// Convert ToolChoice to string
inline std::string ToolChoiceToString(ToolChoice choice) {
    switch (choice) {
        case ToolChoice::kForbidden: return "forbidden";
        case ToolChoice::kAllowed: return "allowed";
        case ToolChoice::kForced: return "forced";
        default: return "allowed";
    }
}

// ============================================================================
// Token and Source Information
// ============================================================================

// Source represents a document source
struct Source {
    std::string uri;  // Document URI (URL or file path)
};

// PromptTokenDetails details of prompt token usage
struct PromptTokenDetails {
    int cached_tokens = 0;  // Cached tokens in the prompt
};

// TokenUsage tracks token consumption
struct TokenUsage {
    int prompt_tokens = 0;             // Tokens used in prompt
    int completion_tokens = 0;         // Tokens used in completion
    int total_tokens = 0;              // Total tokens used
    PromptTokenDetails prompt_token_details;  // Prompt token details
};

// ============================================================================
// Tool Call Information
// ============================================================================

// FunctionCall is the function call in a message
struct FunctionCall {
    std::string name;        // Name of the function to call
    std::string arguments;   // Arguments in JSON format
};

// ToolCall is the tool call in a message
struct ToolCall {
    int* index;                // Index for multiple tool calls (nullptr if not set)
    std::string id;            // ID of the tool call
    std::string type;          // Type of the tool call (default: "function")
    FunctionCall function;     // Function call to be made
    std::map<std::string, json> extra;  // Extra information
    
    ToolCall() : index(nullptr) {}
};

// ============================================================================
// Log Probabilities
// ============================================================================

// TopLogProb represents the top probability for a token
struct TopLogProb {
    std::string token;            // Token text
    double logprob = 0.0;         // Log probability
    std::vector<int64_t> bytes;   // UTF-8 bytes representation
};

// LogProb represents probability information for a token
struct LogProb {
    std::string token;            // Token text
    double logprob = 0.0;         // Log probability
    std::vector<int64_t> bytes;   // UTF-8 bytes representation
    std::vector<TopLogProb> top_logprobs;  // Top log probabilities
};

// LogProbs represents log probability information for a message
struct LogProbs {
    std::vector<LogProb> content;  // Log probabilities for each token
};

// ResponseMeta collects metadata about a chat response
struct ResponseMeta {
    std::string finish_reason;            // Reason for finishing (e.g., "stop", "length", "tool_calls")
    std::shared_ptr<TokenUsage> usage;    // Token usage information
    std::shared_ptr<LogProbs> logprobs;   // Log probability information
};

// ============================================================================
// Multimodal Message Parts
// ============================================================================

// MessagePartCommon represents common fields for multimodal message parts
struct MessagePartCommon {
    std::string* url = nullptr;           // URL or RFC-2397 data URL
    std::string* base64_data = nullptr;   // Base64 encoded data
    std::string mime_type;                // MIME type (e.g., "image/png", "audio/wav")
    std::map<std::string, json> extra;    // Extra information
    
    MessagePartCommon() = default;
    ~MessagePartCommon() {
        delete url;
        delete base64_data;
    }
};

// MessageInputImage represents an image part in user input
struct MessageInputImage {
    MessagePartCommon common;
    ImageURLDetail detail = ImageURLDetail::kAuto;
};

// MessageInputAudio represents an audio part in user input
struct MessageInputAudio {
    MessagePartCommon common;
};

// MessageInputVideo represents a video part in user input
struct MessageInputVideo {
    MessagePartCommon common;
};

// MessageInputFile represents a file part in user input
struct MessageInputFile {
    MessagePartCommon common;
};

// MessageInputPart represents a part of user input (text or multimodal)
struct MessageInputPart {
    ChatMessagePartType type = ChatMessagePartType::kText;
    std::string text;                      // For text parts
    std::shared_ptr<MessageInputImage> image;    // For image parts
    std::shared_ptr<MessageInputAudio> audio;    // For audio parts
    std::shared_ptr<MessageInputVideo> video;    // For video parts
    std::shared_ptr<MessageInputFile> file;      // For file parts
};

// MessageOutputImage represents an image part in model output
struct MessageOutputImage {
    MessagePartCommon common;
};

// MessageOutputAudio represents an audio part in model output
struct MessageOutputAudio {
    MessagePartCommon common;
};

// MessageOutputVideo represents a video part in model output
struct MessageOutputVideo {
    MessagePartCommon common;
};

// MessageOutputPart represents a part of model-generated output
struct MessageOutputPart {
    ChatMessagePartType type = ChatMessagePartType::kText;
    std::string text;                       // For text parts
    std::shared_ptr<MessageOutputImage> image;   // For image parts
    std::shared_ptr<MessageOutputAudio> audio;   // For audio parts
    std::shared_ptr<MessageOutputVideo> video;   // For video parts
};

// Deprecated: ChatMessageImageURL - use MessageInputImage/MessageOutputImage instead
struct ChatMessageImageURL {
    std::string url;       // URL or RFC-2397 data URL
    std::string uri;       // Alternative URI field
    ImageURLDetail detail = ImageURLDetail::kAuto;
    std::string mime_type;
    std::map<std::string, json> extra;
};

// Deprecated: ChatMessageAudioURL - use MessageInputAudio/MessageOutputAudio instead
struct ChatMessageAudioURL {
    std::string url;       // URL or RFC-2397 data URL
    std::string uri;       // Alternative URI field
    std::string mime_type;
    std::map<std::string, json> extra;
};

// Deprecated: ChatMessageVideoURL - use MessageInputVideo/MessageOutputVideo instead
struct ChatMessageVideoURL {
    std::string url;       // URL or RFC-2397 data URL
    std::string uri;       // Alternative URI field
    std::string mime_type;
    std::map<std::string, json> extra;
};

// Deprecated: ChatMessageFileURL - use MessageInputFile instead
struct ChatMessageFileURL {
    std::string url;       // URL or RFC-2397 data URL
    std::string uri;       // Alternative URI field
    std::string mime_type;
    std::string name;      // File name
    std::map<std::string, json> extra;
};

// Deprecated: ChatMessagePart - use MessageInputPart/MessageOutputPart instead
struct ChatMessagePart {
    ChatMessagePartType type = ChatMessagePartType::kText;
    std::string text;
    std::shared_ptr<ChatMessageImageURL> image_url;
    std::shared_ptr<ChatMessageAudioURL> audio_url;
    std::shared_ptr<ChatMessageVideoURL> video_url;
    std::shared_ptr<ChatMessageFileURL> file_url;
};

// ============================================================================
// Message
// ============================================================================

// Message represents a single message in a conversation
// Supports both text-only and multimodal content
struct Message {
    RoleType role;
    std::string content;  // Text content
    
    // Multimodal content (new style)
    std::vector<MessageInputPart> user_input_multi_content;    // User multimodal input
    std::vector<MessageOutputPart> assistant_gen_multi_content;  // Assistant multimodal output
    
    // Deprecated multimodal content (old style)
    std::vector<ChatMessagePart> multi_content;
    
    std::vector<ToolCall> tool_calls;      // Tool calls made by the assistant
    std::string tool_call_id;              // ID of the tool call (for tool messages)
    std::string tool_name;                 // Name of the tool (for tool messages)
    std::string name;                      // Name field
    std::string reasoning_content;         // Reasoning/thinking content from the model
    
    std::shared_ptr<ResponseMeta> response_meta;  // Response metadata
    std::map<std::string, json> extra;    // Extra information
    
    Message() : role(RoleType::kUser), content("") {}
    
    Message(RoleType r, const std::string& c) 
        : role(r), content(c) {}
    
    Message(RoleType r, const std::string& c, const std::vector<ToolCall>& tc) 
        : role(r), content(c), tool_calls(tc) {}
    
    // Get role as string
    std::string GetRoleString() const {
        return RoleTypeToString(role);
    }
    
    // String representation
    std::string String() const;
};

// ============================================================================
// Document
// ============================================================================

// Document represents a single document chunk
struct Document {
    std::string id;              // Document ID
    std::string page_content;    // Document content (also called Content in Go)
    std::map<std::string, json> metadata;  // Document metadata
    
    Document() = default;
    
    Document(const std::string& content) 
        : page_content(content) {}
    
    Document(const std::string& id, const std::string& content) 
        : id(id), page_content(content) {}
    
    // Helper methods for metadata
    void SetMetadata(const std::string& key, const json& value) {
        metadata[key] = value;
    }
    
    json GetMetadata(const std::string& key) const {
        auto it = metadata.find(key);
        return it != metadata.end() ? it->second : json();
    }
    
    // Metadata constants
    static constexpr const char* kScoreKey = "_score";
    static constexpr const char* kSubIndexesKey = "_sub_indexes";
    static constexpr const char* kExtraInfoKey = "_extra_info";
    static constexpr const char* kDenseVectorKey = "_dense_vector";
    static constexpr const char* kSparseVectorKey = "_sparse_vector";
    static constexpr const char* kDSLInfoKey = "_dsl";
    
    // Score management
    Document& WithScore(double score) {
        SetMetadata(kScoreKey, score);
        return *this;
    }
    
    double GetScore() const {
        auto score_val = GetMetadata(kScoreKey);
        if (score_val.is_number()) {
            return static_cast<double>(score_val);
        }
        return 0.0;
    }
    
    // SubIndexes management - stores as raw JSON
    Document& WithSubIndexes(const json& indexes) {
        SetMetadata(kSubIndexesKey, indexes);
        return *this;
    }
    
    json GetSubIndexes() const {
        return GetMetadata(kSubIndexesKey);
    }
    
    // ExtraInfo management
    Document& WithExtraInfo(const std::string& extra_info) {
        SetMetadata(kExtraInfoKey, extra_info);
        return *this;
    }
    
    std::string GetExtraInfo() const {
        auto info = GetMetadata(kExtraInfoKey);
        if (info.is_string()) {
            return static_cast<std::string>(info);
        }
        return "";
    }
    
    // DSLInfo management (stores as json directly)
    Document& WithDSLInfo(const json& dsl_info) {
        SetMetadata(kDSLInfoKey, dsl_info);
        return *this;
    }
    
    json GetDSLInfo() const {
        return GetMetadata(kDSLInfoKey);
    }
    
    // DenseVector management (stores as raw json)
    Document& WithDenseVector(const json& vector) {
        SetMetadata(kDenseVectorKey, vector);
        return *this;
    }
    
    json GetDenseVector() const {
        return GetMetadata(kDenseVectorKey);
    }
    
    // SparseVector management (stores as json directly - map-like object)
    Document& WithSparseVector(const json& sparse) {
        SetMetadata(kSparseVectorKey, sparse);
        return *this;
    }
    
    json GetSparseVector() const {
        return GetMetadata(kSparseVectorKey);
    }
};

// ============================================================================
// Tool Information
// ============================================================================

// ParameterInfo describes a single parameter
struct ParameterInfo {
    DataType type;
    std::string description;
    bool required = false;
    std::vector<std::string> enum_values;
    std::shared_ptr<ParameterInfo> elem_info;  // For array types
    std::map<std::string, std::shared_ptr<ParameterInfo>> sub_params;  // For object types
};

// ParamsOneOf represents tool parameters in one of two formats:
// 1. ParameterInfo-based (intuitive way)
// 2. JSON Schema-based (formal way)
struct ParamsOneOf {
    std::map<std::string, std::shared_ptr<ParameterInfo>> params;  // Parameter info map
    json json_schema;  // Raw JSON schema
    bool has_params = false;  // Whether to use params (true) or json_schema (false)
    
    ParamsOneOf() = default;
    
    // Create from ParameterInfo
    static ParamsOneOf FromParams(const std::map<std::string, std::shared_ptr<ParameterInfo>>& p) {
        ParamsOneOf po;
        po.params = p;
        po.has_params = true;
        return po;
    }
    
    // Create from JSON Schema
    static ParamsOneOf FromJSONSchema(const json& schema) {
        ParamsOneOf po;
        po.json_schema = schema;
        po.has_params = false;
        return po;
    }
};

// ToolInfo represents tool metadata
struct ToolInfo {
    std::string name;           // Name of the tool
    std::string description;    // Description of the tool
    std::shared_ptr<ParamsOneOf> params;  // Parameter schema (optional)
    std::map<std::string, json> extra;    // Extra information
};

// Tool represents a callable tool/function (legacy)
struct Tool {
    std::string name;           // Name of the tool
    std::string description;    // Description of the tool
    json input_schema;          // Input schema in JSON format
    
    // Convert to ToolInfo
    ToolInfo ToToolInfo() const {
        ToolInfo info;
        info.name = name;
        info.description = description;
        info.extra["input_schema"] = input_schema;
        return info;
    }
};

// ============================================================================
// Stream Reader (Basic Interface)
// ============================================================================

// StreamReader represents a stream of items that can be read sequentially
template<typename T>
class StreamReader {
public:
    virtual ~StreamReader() = default;
    
    // Read next item from stream
    // Returns true if item was read, false if stream is exhausted
    virtual bool Next(T& item) = 0;
    
    // Read all items from stream
    virtual std::vector<T> ReadAll() {
        std::vector<T> items;
        T item;
        while (Next(item)) {
            items.push_back(item);
        }
        return items;
    }
    
    // Close the stream
    virtual void Close() = 0;
};

// ============================================================================
// Utility Functions for Creating Messages
// ============================================================================

inline Message SystemMessage(const std::string& content) {
    return Message(RoleType::kSystem, content);
}

inline Message UserMessage(const std::string& content) {
    return Message(RoleType::kUser, content);
}

inline Message AssistantMessage(const std::string& content) {
    return Message(RoleType::kAssistant, content);
}

inline Message ToolMessage(const std::string& content) {
    return Message(RoleType::kTool, content);
}

} // namespace schema
} // namespace eino

#endif // EINO_CPP_SCHEMA_TYPES_H_
