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

#ifndef EINO_CPP_ADK_MIDDLEWARES_SUMMARIZATION_H_
#define EINO_CPP_ADK_MIDDLEWARES_SUMMARIZATION_H_

// Summarization middleware that automatically summarizes conversation history
// when token count exceeds the configured threshold.
// Aligned with Go: adk/middlewares/summarization/summarization.go

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "eino/adk/handler.h"
#include "eino/components/model.h"
#include "eino/schema/types.h"

namespace eino {
namespace adk {
namespace middlewares {
namespace summarization {

// Forward declarations
struct Config;

// TokenCounterInput is the input for TokenCounterFunc.
// Aligned with Go: summarization.TokenCounterInput
struct TokenCounterInput {
    std::vector<schema::Message*> messages;
    std::vector<std::shared_ptr<schema::ToolInfo>> tools;
};

// TokenCounterFunc calculates the token count for given messages and tools.
// Aligned with Go: summarization.TokenCounterFunc
using TokenCounterFunc = std::function<std::pair<int, std::string>(const TokenCounterInput& input)>;

// FinalizeFunc is called after summary generation.
// Aligned with Go: summarization.FinalizeFunc
using FinalizeFunc = std::function<std::pair<std::vector<schema::Message*>, std::string>(
    const std::vector<schema::Message*>& original_messages,
    schema::Message* summary)>;

// CallbackFunc is called after Finalize, before exiting the middleware.
// Aligned with Go: summarization.CallbackFunc
using CallbackFunc = std::function<std::string(
    const ChatModelAgentState& before,
    const ChatModelAgentState& after)>;

// UserMessageFilterFunc determines whether a specific user message should be preserved.
// Aligned with Go: summarization.UserMessageFilterFunc
using UserMessageFilterFunc = std::function<std::pair<bool, std::string>(schema::Message* msg)>;

// TriggerCondition specifies when summarization should be activated.
// Aligned with Go: summarization.TriggerCondition
struct TriggerCondition {
    // ContextTokens triggers summarization when total token count exceeds this threshold.
    int context_tokens = 0;
    // ContextMessages triggers summarization when total messages count exceeds this threshold.
    int context_messages = 0;
};

// PreserveUserMessages controls whether to preserve original user messages in the summary.
// Aligned with Go: summarization.PreserveUserMessages
struct PreserveUserMessages {
    bool enabled = true;
    // MaxTokens limits the maximum token count for preserved user messages.
    int max_tokens = 0;
    // Filter determines whether a specific user message should be preserved.
    UserMessageFilterFunc filter;
};

// RetryConfig configures retry behavior for summary generation.
// Aligned with Go: summarization.RetryConfig
struct RetryConfig {
    int max_retries = 3;
    std::function<bool(schema::Message* resp, const std::string& err)> should_retry;
    std::function<std::chrono::milliseconds(int attempt, schema::Message* resp, const std::string& err)> backoff_func;
};

// FailoverContext contains the state for a failover attempt.
// Aligned with Go: summarization.FailoverContext
struct FailoverContext {
    int attempt = 0;
    schema::Message* system_instruction = nullptr;
    schema::Message* user_instruction = nullptr;
    std::vector<schema::Message*> original_messages;
    schema::Message* last_model_response = nullptr;
    std::string last_err;
};

// GetFailoverModelFunc selects the model and input messages for failover.
// Aligned with Go: summarization.GetFailoverModelFunc
using GetFailoverModelFunc = std::function<
    std::tuple<std::shared_ptr<components::BaseChatModel>,
               std::vector<schema::Message*>,
               std::string>(const FailoverContext& ctx)>;

// FailoverConfig configures fallback behavior when summary generation fails.
// Aligned with Go: summarization.FailoverConfig
struct FailoverConfig {
    int max_retries = 3;
    std::function<bool(schema::Message* resp, const std::string& err)> should_failover;
    std::function<std::chrono::milliseconds(int attempt, schema::Message* resp, const std::string& err)> backoff_func;
    GetFailoverModelFunc get_failover_model;
};

// GenModelInputFunc allows full control over the summarization model input construction.
// Aligned with Go: summarization.GenModelInputFunc
using GenModelInputFunc = std::function<std::pair<std::vector<schema::Message*>, std::string>(
    schema::Message* sys_instruction,
    schema::Message* user_instruction,
    const std::vector<schema::Message*>& original_msgs)>;

// Config defines the configuration for the summarization middleware.
// Aligned with Go: summarization.Config
struct Config {
    // Model is the chat model used to generate summaries. Required.
    std::shared_ptr<components::BaseChatModel> model;

    // TokenCounter calculates the token count. Optional.
    TokenCounterFunc token_counter;

    // Trigger specifies the conditions that activate summarization. Optional.
    std::shared_ptr<TriggerCondition> trigger;

    // EmitInternalEvents indicates whether internal events should be emitted. Optional.
    bool emit_internal_events = false;

    // UserInstruction serves as the user-level instruction for summarization. Optional.
    std::string user_instruction;

    // TranscriptFilePath is the path to the full conversation history file. Optional.
    std::string transcript_file_path;

    // GenModelInput allows full control over model input construction. Optional.
    GenModelInputFunc gen_model_input;

    // Finalize is called after summary generation. Optional.
    FinalizeFunc finalize;

    // Callback is called after Finalize. Optional.
    CallbackFunc callback;

    // PreserveUserMessages controls whether to preserve original user messages. Optional.
    std::shared_ptr<PreserveUserMessages> preserve_user_messages;

    // Retry configures retry behavior. Optional.
    std::shared_ptr<RetryConfig> retry;

    // Failover configures fallback behavior. Optional.
    std::shared_ptr<FailoverConfig> failover;
};

// SummarizeOutput contains the output of a synchronous Summarize call.
// Aligned with Go: summarization.SummarizeOutput
struct SummarizeOutput {
    std::vector<schema::Message*> finalized_messages;
    schema::Message* model_response = nullptr;
};

// New creates a summarization middleware.
// Aligned with Go: summarization.New()
std::pair<std::shared_ptr<ChatModelAgentMiddleware>, std::string> New(const Config& config);

// SummarizeMessages performs synchronous summarization of the given messages.
// Aligned with Go: summarization.SummarizeMessages()
std::pair<std::shared_ptr<SummarizeOutput>, std::string> SummarizeMessages(
    const Config& config,
    const std::vector<schema::Message*>& messages);

}  // namespace summarization
}  // namespace middlewares
}  // namespace adk
}  // namespace eino

#endif  // EINO_CPP_ADK_MIDDLEWARES_SUMMARIZATION_H_
