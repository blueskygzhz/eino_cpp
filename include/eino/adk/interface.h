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

#ifndef EINO_CPP_ADK_INTERFACE_H_
#define EINO_CPP_ADK_INTERFACE_H_

// ADK (Application Development Kit) module
// Provides high-level abstractions for building AI applications with agents
//
// ARCHITECTURE NOTE (Alignment with eino Go):
// ==========================================
// ADK is built on top of Compose's Runnable framework:
// 
// - Agent interface wraps Compose Runnable
// - Agent::Run() internally delegates to Compose Runnable stream execution
// - Workflows (Sequential, Loop, Parallel) use Compose Graph/Chain for orchestration
// - State management uses Compose State for data flow between agents
//
// This architecture enables:
// 1. Seamless interoperability between ADK agents and Compose chains
// 2. Consistent callback and streaming semantics
// 3. Type-safe composition of agents within larger workflows
// 4. Parity with eino (Go) implementation

#include <string>
#include <map>
#include <memory>
#include <vector>
#include \"types.h\"
#include \"agent.h\"
#include \"agent_base.h\"
#include \"flow_agent.h\"
#include \"async_iterator.h\"
#include \"call_options.h\"
#include \"chat_model_agent.h\"
#include \"flow.h\"
#include \"runner.h\"
#include \"agent_tool.h\"
#include \"context.h\"
#include \"workflow.h\"
#include \"utils.h\"
#include \"executor.h\"
#include \"prebuilt/plan_execute.h\"
#include "../compose/runnable.h"
#include "../compose/state.h"
#include "../compose/graph.h"
#include "../compose/chain.h"

namespace eino {
namespace adk {

// ============================================================================
// Core ADK Functions - Compose-based Agent Management
// ============================================================================

// Session management functions
// These manage context-local session data during agent execution
// Uses Compose Context for storage

// GetSessionValues retrieves all session values from context
std::map<std::string, void*> GetSessionValues(void* ctx);

// AddSessionValue adds or updates a session value
void AddSessionValue(void* ctx, const std::string& key, void* value);

// GetSessionValue retrieves a specific session value
void* GetSessionValue(void* ctx, const std::string& key, bool* found = nullptr);

// ClearRunContext clears the run context (useful for nested agent execution)
void* ClearRunContext(void* ctx);

// ============================================================================
// Tool Utility Functions
// ============================================================================

// GetTransferToAgentToolName returns predefined tool name for agent transfer
const std::string& GetTransferToAgentToolName();

// GetTransferToAgentToolDesc returns description of transfer tool
const std::string& GetTransferToAgentToolDesc();

// GetExitToolName returns predefined tool name for agent exit
const std::string& GetExitToolName();

// GetExitToolDesc returns description of exit tool
const std::string& GetExitToolDesc();

// NewExitTool creates an exit tool action
void* NewExitTool();

// NewTransferToAgentAction creates a transfer-to-agent action
void* NewTransferToAgentAction(const std::string& dest_agent_name);

// ============================================================================
// Helper Type Definitions for Compose-based Agents
// ============================================================================

// All agents must:
// 1. Inherit from Agent interface
// 2. Implement via AgentBase which wraps a Compose Runnable
// 3. Use Compose Graph/Chain for internal orchestration
// 4. Leverage Compose callbacks and streaming

typedef std::shared_ptr<Agent> AgentPtr;
typedef std::shared_ptr<FlowAgent> FlowAgentPtr;
typedef std::shared_ptr<Runner> RunnerPtr;

// ============================================================================
// Compose Integration Points for ADK
// ============================================================================

// Agents can be wrapped as Runnables for use in Compose chains/graphs
// This enables composing ADK agents with other Compose components

template<typename InputType, typename OutputType>
std::shared_ptr<compose::Runnable<InputType, OutputType>>
WrapAgentAsRunnable(std::shared_ptr<Agent> agent);

}  // namespace adk
}  // namespace eino

#endif  // EINO_CPP_ADK_INTERFACE_H_
