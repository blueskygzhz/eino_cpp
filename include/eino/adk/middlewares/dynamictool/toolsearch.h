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

#ifndef EINO_CPP_ADK_MIDDLEWARES_DYNAMICTOOL_TOOLSEARCH_H_
#define EINO_CPP_ADK_MIDDLEWARES_DYNAMICTOOL_TOOLSEARCH_H_

// Dynamic tool search middleware.
// Aligned with Go: adk/middlewares/dynamictool/toolsearch/toolsearch.go
//
// Enables dynamic tool selection for agents with large tool libraries.
// Instead of passing all tools to the model at once, this middleware:
//   1. Adds a "tool_search" meta-tool that accepts a regex pattern
//   2. Initially hides all dynamic tools from the model's tool list
//   3. When the model calls tool_search, matching tools become available

#include <memory>
#include <string>
#include <vector>

#include "eino/adk/handler.h"
#include "eino/components/tool.h"

namespace eino {
namespace adk {
namespace middlewares {
namespace dynamictool {
namespace toolsearch {

// Config is the configuration for the tool search middleware.
// Aligned with Go: toolsearch.Config
struct Config {
    // DynamicTools is a list of tools that can be dynamically searched and loaded.
    std::vector<std::shared_ptr<components::BaseTool>> dynamic_tools;
};

// New constructs and returns the tool search middleware.
// Aligned with Go: toolsearch.New()
std::pair<std::shared_ptr<ChatModelAgentMiddleware>, std::string> New(const Config& config);

}  // namespace toolsearch
}  // namespace dynamictool
}  // namespace middlewares
}  // namespace adk
}  // namespace eino

#endif  // EINO_CPP_ADK_MIDDLEWARES_DYNAMICTOOL_TOOLSEARCH_H_
