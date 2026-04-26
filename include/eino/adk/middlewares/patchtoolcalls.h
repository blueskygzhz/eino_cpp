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

#ifndef EINO_CPP_ADK_MIDDLEWARES_PATCHTOOLCALLS_H_
#define EINO_CPP_ADK_MIDDLEWARES_PATCHTOOLCALLS_H_

// Patch tool calls middleware that patches dangling tool calls in message history.
// Aligned with Go: adk/middlewares/patchtoolcalls/patchtoolcalls.go
//
// The middleware scans the message history before each model invocation and inserts
// placeholder tool messages for any tool calls that don't have corresponding responses.

#include <functional>
#include <memory>
#include <string>

#include "eino/adk/handler.h"

namespace eino {
namespace adk {
namespace middlewares {
namespace patchtoolcalls {

// PatchedContentGenerator is an optional custom function to generate the content
// of patched tool messages.
// Aligned with Go: patchtoolcalls.Config.PatchedContentGenerator
using PatchedContentGenerator = std::function<std::pair<std::string, std::string>(
    const std::string& tool_name,
    const std::string& tool_call_id)>;

// Config defines the configuration options for the patch tool calls middleware.
// Aligned with Go: patchtoolcalls.Config
struct Config {
    // PatchedContentGenerator is an optional custom function to generate the content
    // of patched tool messages. If not provided, a default message will be used.
    PatchedContentGenerator patched_content_generator;
};

// New creates a new patch tool calls middleware with the given configuration.
// Aligned with Go: patchtoolcalls.New()
std::pair<std::shared_ptr<ChatModelAgentMiddleware>, std::string> New(
    const Config& config = Config{});

}  // namespace patchtoolcalls
}  // namespace middlewares
}  // namespace adk
}  // namespace eino

#endif  // EINO_CPP_ADK_MIDDLEWARES_PATCHTOOLCALLS_H_
