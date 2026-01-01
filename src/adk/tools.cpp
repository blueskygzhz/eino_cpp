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

#include "eino/adk/tools.h"
#include "eino/adk/context.h"
#include "eino/adk/react.h"
#include <nlohmann/json.hpp>
#include <sstream>

namespace eino {
namespace adk {

// =============================================================================
// TransferToAgentTool Implementation
// Aligns with: eino/adk/chatmodel.go:320-347
// =============================================================================

std::string TransferToAgentTool::InvokableRun(
    void* ctx,
    const std::string& arguments_json,
    const std::vector<components::ToolOption>& options) {
    
    // Parse arguments - aligns with chatmodel.go:335-339
    nlohmann::json params;
    try {
        params = nlohmann::json::parse(arguments_json);
    } catch (const std::exception& e) {
        return std::string("Error parsing arguments: ") + e.what();
    }
    
    if (!params.contains("agent_name")) {
        return "Error: missing required parameter 'agent_name'";
    }
    
    std::string dest_agent_name = params["agent_name"].get<std::string>();
    
    // Send tool generated action - aligns with chatmodel.go:341-344
    auto action = NewTransferToAgentAction(dest_agent_name);
    auto err = SendToolGenAction(ctx, kToolName, action);
    if (!err.empty()) {
        return "Error sending transfer action: " + err;
    }
    
    // Return success message - aligns with chatmodel.go:346
    std::ostringstream oss;
    oss << "successfully transferred to agent [" << dest_agent_name << "]";
    return oss.str();
}

// =============================================================================
// ExitTool Implementation
// Aligns with: eino/adk/chatmodel.go:295-318
// =============================================================================

std::string ExitTool::InvokableRun(
    void* ctx,
    const std::string& arguments_json,
    const std::vector<components::ToolOption>& options) {
    
    // Parse arguments - aligns with chatmodel.go:306-310
    nlohmann::json params;
    try {
        params = nlohmann::json::parse(arguments_json);
    } catch (const std::exception& e) {
        return std::string("Error parsing arguments: ") + e.what();
    }
    
    if (!params.contains("final_result")) {
        return "Error: missing required parameter 'final_result'";
    }
    
    std::string final_result = params["final_result"].get<std::string>();
    
    // Send exit action - aligns with chatmodel.go:312-315
    auto action = NewExitAction();
    auto err = SendToolGenAction(ctx, kToolName, action);
    if (!err.empty()) {
        return "Error sending exit action: " + err;
    }
    
    // Return final result - aligns with chatmodel.go:317
    return final_result;
}

// =============================================================================
// Action Helpers
// Aligns with: eino/adk/interface.go
// =============================================================================

std::shared_ptr<AgentAction> NewTransferToAgentAction(const std::string& dest_agent_name) {
    auto action = std::make_shared<AgentAction>();
    action->transfer_to_agent = std::make_shared<TransferToAgentAction>();
    action->transfer_to_agent->dest_agent_name = dest_agent_name;
    return action;
}

std::shared_ptr<AgentAction> NewExitAction() {
    auto action = std::make_shared<AgentAction>();
    action->exit = true;
    return action;
}

// =============================================================================
// SendToolGenAction / PopToolGenAction
// Aligns with: eino/adk/react.go:51-73
// =============================================================================

std::string SendToolGenAction(
    void* ctx,
    const std::string& tool_name,
    std::shared_ptr<AgentAction> action) {
    
    auto state = GetOrCreateReactState(ctx);
    if (!state) {
        return "failed to get state";
    }
    
    state->tool_gen_actions[tool_name] = action;
    return "";  // Empty string means success
}

std::shared_ptr<AgentAction> PopToolGenAction(
    void* ctx,
    const std::string& tool_name) {
    
    auto state = GetOrCreateReactState(ctx);
    if (!state) {
        return nullptr;
    }
    
    auto it = state->tool_gen_actions.find(tool_name);
    if (it == state->tool_gen_actions.end()) {
        return nullptr;
    }
    
    auto action = it->second;
    state->tool_gen_actions.erase(it);
    
    return action;
}

}  // namespace adk
}  // namespace eino
