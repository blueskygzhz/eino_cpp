/*
 * Copyright 2025 CloudWeGo Authors
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

#ifndef EINO_CPP_SCHEMA_TOOL_RESULT_H_
#define EINO_CPP_SCHEMA_TOOL_RESULT_H_

// ToolArgument and ToolResult types for enhanced tool interfaces.
// Aligned with Go: schema.ToolArgument, schema.ToolResult, schema.ToolOutputPart

#include <any>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace eino {
namespace schema {

// ToolPartType defines the type of a tool output part.
// Aligned with Go: schema.ToolPartType*
enum class ToolPartType {
    kText,
    kImage,
    kAudio,
    kVideo,
    kFile,
};

// MessagePartCommon holds common fields for multimodal message parts.
// Aligned with Go: schema.MessagePartCommon
struct MessagePartCommon {
    std::string mime_type;
    std::string data;       // Base64-encoded data
    std::string url;        // URL to the resource
};

// ToolOutputImage represents an image output from a tool.
// Aligned with Go: schema.ToolOutputImage
struct ToolOutputImage {
    MessagePartCommon common;
};

// ToolOutputAudio represents an audio output from a tool.
// Aligned with Go: schema.ToolOutputAudio
struct ToolOutputAudio {
    MessagePartCommon common;
};

// ToolOutputVideo represents a video output from a tool.
// Aligned with Go: schema.ToolOutputVideo
struct ToolOutputVideo {
    MessagePartCommon common;
};

// ToolOutputFile represents a file output from a tool.
// Aligned with Go: schema.ToolOutputFile
struct ToolOutputFile {
    MessagePartCommon common;
};

// ToolOutputPart represents a single part of a tool's multimodal output.
// Aligned with Go: schema.ToolOutputPart
struct ToolOutputPart {
    ToolPartType type;
    std::string text;                            // For kText type
    std::shared_ptr<ToolOutputImage> image;      // For kImage type
    std::shared_ptr<ToolOutputAudio> audio;      // For kAudio type
    std::shared_ptr<ToolOutputVideo> video;      // For kVideo type
    std::shared_ptr<ToolOutputFile> file;        // For kFile type
    std::map<std::string, std::any> extra;       // Extra information
};

// ToolArgument contains the input information for a tool call.
// It is used to pass tool call arguments to enhanced tools.
// Aligned with Go: schema.ToolArgument
struct ToolArgument {
    // Text contains the arguments for the tool call in JSON format.
    std::string text;
};

// ToolResult represents the structured multimodal output from a tool execution.
// It is used when a tool needs to return more than just a simple string,
// such as images, files, or other structured data.
// Aligned with Go: schema.ToolResult
struct ToolResult {
    // Parts contains the multimodal output parts.
    std::vector<ToolOutputPart> parts;
};

// ConcatToolResults concatenates multiple ToolResult chunks into a single ToolResult.
// Aligned with Go: schema.ConcatToolResults()
inline ToolResult ConcatToolResults(const std::vector<std::shared_ptr<ToolResult>>& chunks) {
    ToolResult result;
    for (const auto& chunk : chunks) {
        if (chunk) {
            for (const auto& part : chunk->parts) {
                result.parts.push_back(part);
            }
        }
    }
    return result;
}

}  // namespace schema
}  // namespace eino

#endif  // EINO_CPP_SCHEMA_TOOL_RESULT_H_
