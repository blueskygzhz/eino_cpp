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

#ifndef EINO_CPP_ADK_UTILS_H_
#define EINO_CPP_ADK_UTILS_H_

// ADK Utility Functions
// =====================
// Helper functions for agent configuration, options handling,
// and advanced agent patterns (deterministic transfer, agent wrapping)

#include "types.h"
#include "agent.h"
#include "async_iterator.h"
#include <memory>
#include <string>
#include <vector>
#include <map>

namespace eino {
namespace adk {

// ============================================================================
// Agent Configuration Options
// ============================================================================

// WithSessionValues creates option to set session values for agent execution
std::shared_ptr<AgentRunOption> WithSessionValues(const std::map<std::string, void*>& values);

// WithSkipTransferMessages creates option to skip transfer messages
std::shared_ptr<AgentRunOption> WithSkipTransferMessages();

// WithCheckPointID creates option to set checkpoint ID
std::shared_ptr<AgentRunOption> WithCheckPointID(const std::string& id);

// GetCommonOptions extracts common options from option list
struct CommonOptions {
    std::map<std::string, void*> session_values;
    std::string checkpoint_id;
    bool skip_transfer_messages = false;
};

CommonOptions GetCommonOptions(
    const std::vector<std::shared_ptr<AgentRunOption>>& opts);

// ============================================================================
// Message Utilities
// ============================================================================

// Copy agent event (deep copy with proper stream handling)
std::shared_ptr<AgentEvent> CopyAgentEvent(const std::shared_ptr<AgentEvent>& ae);

// Generate transfer messages between agents
// Returns (assistant_message, tool_message)
std::pair<Message, Message> GenTransferMessages(void* ctx, const std::string& dest_agent_name);

// Get message from agent event
// If streaming, concatenates all chunks; if not, returns single message
std::pair<Message, std::string> GetMessageFromEvent(std::shared_ptr<AgentEvent> event);

// SetAutomaticClose sets automatic close on MessageStream in AgentEvent
// Aligns with eino adk.setAutomaticClose (utils.go:83-90)
// This ensures that the stream will be automatically closed when no longer needed
void SetAutomaticClose(std::shared_ptr<AgentEvent> event);

// GetMessageFromWrappedEvent extracts Message from AgentEvent
// Aligns with eino adk.getMessageFromWrappedEvent (utils.go:93-135)
// For streaming events, it concatenates all chunks into a single message
Message GetMessageFromWrappedEvent(const std::shared_ptr<AgentEvent>& event);

// ============================================================================
// Agent Middleware and Configuration
// ============================================================================

// Configure agent with options (history rewriter, etc.)
std::shared_ptr<Agent> ConfigureAgent(
    std::shared_ptr<Agent> agent,
    const std::string& name = "",
    const std::string& description = "",
    HistoryRewriter rewriter = nullptr,
    bool disallow_transfer_to_parent = false);

// ============================================================================
// Deterministic Transfer Pattern
// ============================================================================

// DeterministicTransferConfig configures deterministic agent transfers
struct DeterministicTransferConfig {
    std::shared_ptr<Agent> agent;
    std::vector<std::string> to_agent_names;
};

// AgentWithDeterministicTransferTo wraps an agent to deterministically
// transfer to specified agents after completion
std::shared_ptr<Agent> AgentWithDeterministicTransferTo(
    const DeterministicTransferConfig& config);

// ============================================================================
// Agent Tree Utilities
// ============================================================================

// SetSubAgents sets sub-agents for a parent agent
// Returns error if agents are already configured or if agent doesn't support sub-agents
std::pair<std::shared_ptr<Agent>, std::string> SetSubAgents(
    void* ctx,
    std::shared_ptr<Agent> parent_agent,
    const std::vector<std::shared_ptr<Agent>>& sub_agents);

// GetAgent finds a sub-agent by name (recursive)
std::shared_ptr<Agent> GetAgent(
    std::shared_ptr<Agent> root_agent,
    void* ctx,
    const std::string& name);

// ============================================================================
// Context and Session Utilities
// ============================================================================

// ClearRunContext clears execution context (useful for isolated sub-agent execution)
void* ClearRunContext(void* ctx);

// GenerateTransferToolOutput creates tool output for agent transfer
std::string GenerateTransferToolOutput(const std::string& dest_agent_name);

}  // namespace adk
}  // namespace eino

#endif  // EINO_CPP_ADK_UTILS_H_
