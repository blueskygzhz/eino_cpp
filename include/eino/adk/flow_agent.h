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

#ifndef EINO_CPP_ADK_FLOW_AGENT_H_
#define EINO_CPP_ADK_FLOW_AGENT_H_

// FlowAgent - Multi-Agent Orchestration Built on Compose Framework
// ================================================================
// FlowAgent orchestrates multiple sub-agents using Compose Graph patterns.
// This aligns with eino (Go) architecture where FlowAgent wraps Compose Graph.
//
// Execution modes map to Compose patterns:
// - Sequential: Compose Chain (serial execution)
// - Parallel: Compose parallel edge execution
// - Loop: Compose looping with conditional routing

#include "agent.h"
#include "agent_base.h"
#include "async_iterator.h"
#include "types.h"
#include "../compose/state.h"
#include "../compose/graph.h"
#include "../compose/runnable.h"
#include <memory>
#include <vector>
#include <string>
#include <functional>

namespace eino {
namespace adk {

// HistoryEntry represents an entry in conversation history
struct HistoryEntry {
    bool is_user_input = false;
    std::string agent_name;
    Message message;
};

// HistoryRewriter rewrites conversation history
typedef std::function<std::vector<Message>(void*, const std::vector<HistoryEntry>&)> HistoryRewriter;

// FlowAgent manages multiple sub-agents with configurable execution flow
// ARCHITECTURE: Built on Compose Graph/Chain framework
// - Internal execution uses Compose Runnable/Graph
// - Sub-agents are wrapped as Compose nodes
// - Data flow managed through Compose State
class FlowAgent : public ResumableAgent, public OnSubAgents, 
                  public std::enable_shared_from_this<FlowAgent> {
public:
    FlowAgent();
    virtual ~FlowAgent() = default;

    // Name returns the flow agent name
    std::string Name(void* ctx) override;

    // Description returns the flow agent description
    std::string Description(void* ctx) override;

    // Run executes the flow with sequential/parallel/loop modes
    std::shared_ptr<AsyncIterator<std::shared_ptr<AgentEvent>>> Run(
        void* ctx,
        const std::shared_ptr<AgentInput>& input,
        const std::vector<std::shared_ptr<AgentRunOption>>& options) override;

    // Resume resumes flow execution from interrupt point
    std::shared_ptr<AsyncIterator<std::shared_ptr<AgentEvent>>> Resume(
        void* ctx,
        const std::shared_ptr<ResumeInfo>& info,
        const std::vector<std::shared_ptr<AgentRunOption>>& options) override;

    // OnSetSubAgents sets sub-agents for the flow
    void OnSetSubAgents(void* ctx, const std::vector<std::shared_ptr<Agent>>& agents) override;

    // OnSetAsSubAgent handles being set as a sub-agent
    void OnSetAsSubAgent(void* ctx, const std::shared_ptr<Agent>& parent) override;

    // OnDisallowTransferToParent prevents transfer to parent agent
    void OnDisallowTransferToParent(void* ctx) override;

    // Configuration methods
    void SetName(const std::string& name);
    void SetDescription(const std::string& desc);
    void SetHistoryRewriter(HistoryRewriter rewriter);
    void SetDisallowTransferToParent(bool disallow);
    void SetCheckPointStore(void* store);

    // Getter methods
    std::vector<std::shared_ptr<Agent>> GetSubAgents() const;
    std::shared_ptr<Agent> GetParentAgent() const;
    bool IsTransferToParentDisallowed() const;
    void* GetCheckPointStore() const;
    
    // DeepCopy creates a deep copy of this FlowAgent
    // Aligns with eino adk flowAgent.deepCopy()
    std::shared_ptr<FlowAgent> DeepCopy() const;
    
    // GetAgent recursively searches for an agent by name
    // Searches in: self -> sub-agents -> parent (if allowed)
    // Aligns with eino adk flowAgent.getAgent()
    std::shared_ptr<Agent> GetAgent(void* ctx, const std::string& name);

private:
    std::string name_;
    std::string description_;
    std::vector<std::shared_ptr<Agent>> sub_agents_;
    std::shared_ptr<Agent> parent_agent_;
    bool disallow_transfer_to_parent_ = false;
    HistoryRewriter history_rewriter_;
    void* checkpoint_store_ = nullptr;

    // Execution mode enumeration
    enum ExecutionMode {
        MODE_SEQUENTIAL = 0,
        MODE_PARALLEL = 1,
        MODE_LOOP = 2
    };

    ExecutionMode execution_mode_ = MODE_SEQUENTIAL;

    // Helper methods for execution
    std::shared_ptr<AsyncIterator<std::shared_ptr<AgentEvent>>> ExecuteSequential(
        void* ctx,
        const std::shared_ptr<AgentInput>& input,
        const std::vector<std::shared_ptr<AgentRunOption>>& options);

    std::shared_ptr<AsyncIterator<std::shared_ptr<AgentEvent>>> ExecuteParallel(
        void* ctx,
        const std::shared_ptr<AgentInput>& input,
        const std::vector<std::shared_ptr<AgentRunOption>>& options);

    std::shared_ptr<AsyncIterator<std::shared_ptr<AgentEvent>>> ExecuteLoop(
        void* ctx,
        const std::shared_ptr<AgentInput>& input,
        const std::vector<std::shared_ptr<AgentRunOption>>& options);
    
    // Helper to handle transfer action
    void HandleTransferAction(
        void* ctx,
        std::shared_ptr<AgentAction> action,
        std::shared_ptr<AsyncGenerator<std::shared_ptr<AgentEvent>>> gen,
        const std::vector<std::shared_ptr<AgentRunOption>>& options);
    
    // genAgentInput generates agent input from run context
    // Aligns with eino adk flowAgent.genAgentInput()
    // Go reference: eino/adk/flow.go lines 220-270
    std::shared_ptr<AgentInput> genAgentInput(
        void* ctx,
        std::shared_ptr<ExecutionContext> runCtx,
        bool skipTransferMessages);
    
    // Helper functions for history processing
    static std::vector<Message> DefaultHistoryRewriter(
        void* ctx,
        const std::vector<HistoryEntry>& entries,
        const std::string& agentName);
    
    static Message RewriteMessage(const Message& msg, const std::string& agentName);
    
    static bool BelongToRunPath(
        const std::vector<RunStep>& eventRunPath,
        const std::vector<RunStep>& runPath);
};

// Factory functions
std::shared_ptr<FlowAgent> NewFlowAgent();
std::shared_ptr<FlowAgent> NewFlowAgent(const std::string& name, const std::string& desc);

}  // namespace adk
}  // namespace eino

#endif  // EINO_CPP_ADK_FLOW_AGENT_H_
