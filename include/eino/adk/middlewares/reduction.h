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

#ifndef EINO_CPP_ADK_MIDDLEWARES_REDUCTION_H_
#define EINO_CPP_ADK_MIDDLEWARES_REDUCTION_H_

// Tool reduction middleware for managing tool outputs.
// Aligned with Go: adk/middlewares/reduction/reduction.go
//
// This middleware manages tool outputs in two phases to optimize context usage:
//   1. Truncation Phase: Triggered after tool execution. If output exceeds
//      MaxLengthForTrunc, saves full content to Backend and replaces with truncated notice.
//   2. Clear Phase: Triggered before model call. If total tokens exceed
//      MaxTokensForClear, offloads historical tool results to Backend.

#include <any>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "eino/adk/filesystem/backend.h"
#include "eino/adk/handler.h"
#include "eino/schema/tool_result.h"
#include "eino/schema/types.h"

namespace eino {
namespace adk {
namespace middlewares {
namespace reduction {

// Backend is the storage backend for reduction middleware.
// Re-exports filesystem::Backend for convenience.
using Backend = filesystem::Backend;

// ToolDetail contains information about a tool call for handler processing.
// Aligned with Go: reduction.ToolDetail
struct ToolDetail {
    std::shared_ptr<ToolContext> tool_context;
    std::shared_ptr<schema::ToolArgument> tool_argument;
    std::shared_ptr<schema::ToolResult> tool_result;
    // For streaming results
    std::shared_ptr<schema::StreamReader<std::shared_ptr<schema::ToolResult>>> stream_tool_result;
};

// TruncResult contains the result of truncation processing.
// Aligned with Go: reduction.TruncResult
struct TruncResult {
    bool need_trunc = false;
    std::shared_ptr<schema::ToolResult> tool_result;
    std::shared_ptr<schema::StreamReader<std::shared_ptr<schema::ToolResult>>> stream_tool_result;
    bool need_offload = false;
    std::string offload_file_path;
    std::string offload_content;
};

// ClearResult contains the result of clear processing.
// Aligned with Go: reduction.ClearResult
struct ClearResult {
    bool need_clear = false;
    std::shared_ptr<schema::ToolArgument> tool_argument;
    std::shared_ptr<schema::ToolResult> tool_result;
    bool need_offload = false;
    std::string offload_file_path;
    std::string offload_content;
};

// TruncHandler processes tool call results during truncation.
using TruncHandler = std::function<std::pair<std::shared_ptr<TruncResult>, std::string>(
    const ToolDetail& detail)>;

// ClearHandler processes tool call arguments and results during clearing.
using ClearHandler = std::function<std::pair<std::shared_ptr<ClearResult>, std::string>(
    const ToolDetail& detail)>;

// ToolReductionConfig is the specific configuration for a tool by name.
// Aligned with Go: reduction.ToolReductionConfig
struct ToolReductionConfig {
    std::shared_ptr<Backend> backend;
    bool skip_truncation = false;
    TruncHandler trunc_handler;
    bool skip_clear = false;
    ClearHandler clear_handler;
};

// TokenCounter counts the number of tokens in conversation messages.
// Aligned with Go: reduction.Config.TokenCounter
using TokenCounter = std::function<std::pair<int64_t, std::string>(
    const std::vector<schema::Message*>& msgs,
    const std::vector<std::shared_ptr<schema::ToolInfo>>& tools)>;

// Config is the configuration for tool reduction middleware.
// Aligned with Go: reduction.Config
struct Config {
    // Backend is the storage backend where truncated content will be saved. Required.
    std::shared_ptr<Backend> backend;

    // SkipTruncation skip truncating.
    bool skip_truncation = false;

    // SkipClear skip clearing.
    bool skip_clear = false;

    // ReadFileToolName is tool name used to retrieve from file. Default: "read_file".
    std::string read_file_tool_name = "read_file";

    // RootDir root dir to save truncated/cleared content. Default: "/tmp".
    std::string root_dir = "/tmp";

    // MaxLengthForTrunc is the maximum allowed length of the tool output. Default: 50000.
    int max_length_for_trunc = 50000;

    // TruncExcludeTools is list of tool names whose results should never be truncated.
    std::vector<std::string> trunc_exclude_tools;

    // TokenCounter is used to count tokens. Required.
    TokenCounter token_counter;

    // MaxTokensForClear is the maximum tokens before clearing is attempted. Default: 160000.
    int64_t max_tokens_for_clear = 160000;

    // ClearRetentionSuffixLimit is the number of recent messages to retain. Default: 1.
    int clear_retention_suffix_limit = 1;

    // ClearAtLeastTokens ensures minimum tokens cleared each time. Default: 0.
    int64_t clear_at_least_tokens = 0;

    // ClearExcludeTools is list of tool names whose results should never be cleared.
    std::vector<std::string> clear_exclude_tools;

    // ClearPostProcess is clear post process handler.
    std::function<void(std::shared_ptr<ChatModelAgentState> state)> clear_post_process;

    // ToolConfig is the specific configuration that applies to tools by name.
    std::map<std::string, std::shared_ptr<ToolReductionConfig>> tool_config;
};

// New creates tool reduction middleware from config.
// Aligned with Go: reduction.New()
std::pair<std::shared_ptr<ChatModelAgentMiddleware>, std::string> New(const Config& config);

}  // namespace reduction
}  // namespace middlewares
}  // namespace adk
}  // namespace eino

#endif  // EINO_CPP_ADK_MIDDLEWARES_REDUCTION_H_
