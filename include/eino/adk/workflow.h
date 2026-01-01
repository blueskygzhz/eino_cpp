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

#ifndef EINO_CPP_ADK_WORKFLOW_H_
#define EINO_CPP_ADK_WORKFLOW_H_

// Workflow Agents - Multi-Agent Orchestration Patterns
// ====================================================
// Implements three execution patterns on top of Compose Graph framework:
// 
// 1. SequentialAgent: Execute sub-agents one after another
// 2. ParallelAgent: Execute multiple sub-agents concurrently
// 3. LoopAgent: Repeatedly execute sub-agents until condition met
//
// These align with eino's workflow implementation and use Compose
// Graph internally for orchestration.

#include "agent.h"
#include "flow_agent.h"
#include "types.h"
#include <memory>
#include <vector>
#include <string>
#include <functional>

namespace eino {
namespace adk {

// BreakLoopAction signals a request to terminate a loop
struct BreakLoopAction {
    // From records the name of the agent that initiated the break
    std::string from;
    
    // Done is a state flag that marks when the action has been handled
    bool done = false;
    
    // CurrentIterations records at which iteration the loop was broken
    int current_iterations = 0;
};

// Create a new BreakLoopAction
std::shared_ptr<AgentAction> NewBreakLoopAction(const std::string& agent_name);

// WorkflowInterruptInfo stores interrupt state for workflow agents
struct WorkflowInterruptInfo {
    // Original input to the workflow
    std::shared_ptr<AgentInput> orig_input;
    
    // For sequential: which sub-agent interrupted
    int sequential_interrupt_index = -1;
    std::shared_ptr<InterruptInfo> sequential_interrupt_info;
    
    // For loop: which iteration we're on
    int loop_iterations = 0;
    
    // For parallel: which sub-agents were interrupted
    std::map<int, std::shared_ptr<InterruptInfo>> parallel_interrupt_info;
};

// Base class for workflow agents
// These wrap multiple sub-agents and orchestrate their execution
class WorkflowAgent : public FlowAgent {
public:
    virtual ~WorkflowAgent() = default;

    // GetExecutionMode returns the execution mode
    virtual int GetExecutionMode() const = 0;

    // GetMaxIterations returns max iterations for loop agent (0 = unlimited)
    virtual int GetMaxIterations() const { return 0; }

protected:
    // Helper to execute sub-agents sequentially
    // Returns (exit, interrupted)
    std::pair<bool, bool> ExecuteSequentialInternal(
        void* ctx,
        const std::shared_ptr<AgentInput>& input,
        const std::vector<std::shared_ptr<AgentRunOption>>& options,
        std::shared_ptr<AsyncGenerator<std::shared_ptr<AgentEvent>>> gen,
        const std::shared_ptr<WorkflowInterruptInfo>& interrupt_info = nullptr,
        int iterations = 0);

    // Helper to execute sub-agents in parallel
    void ExecuteParallelInternal(
        void* ctx,
        const std::shared_ptr<AgentInput>& input,
        const std::vector<std::shared_ptr<AgentRunOption>>& options,
        std::shared_ptr<AsyncGenerator<std::shared_ptr<AgentEvent>>> gen,
        const std::shared_ptr<WorkflowInterruptInfo>& interrupt_info = nullptr);

    // Helper to execute loop with sequential pattern
    void ExecuteLoopInternal(
        void* ctx,
        const std::shared_ptr<AgentInput>& input,
        const std::vector<std::shared_ptr<AgentRunOption>>& options,
        std::shared_ptr<AsyncGenerator<std::shared_ptr<AgentEvent>>> gen,
        const std::shared_ptr<WorkflowInterruptInfo>& interrupt_info = nullptr);

    // Check if break loop action should terminate
    bool CheckBreakLoop(std::shared_ptr<AgentAction> action, int iterations);
};

// SequentialAgent executes sub-agents one after another
class SequentialAgent : public WorkflowAgent {
public:
    SequentialAgent();
    explicit SequentialAgent(const std::string& name, const std::string& description = "");
    virtual ~SequentialAgent() = default;

    std::shared_ptr<AsyncIterator<std::shared_ptr<AgentEvent>>> Run(
        void* ctx,
        const std::shared_ptr<AgentInput>& input,
        const std::vector<std::shared_ptr<AgentRunOption>>& options) override;

    std::shared_ptr<AsyncIterator<std::shared_ptr<AgentEvent>>> Resume(
        void* ctx,
        const std::shared_ptr<ResumeInfo>& info,
        const std::vector<std::shared_ptr<AgentRunOption>>& options) override;

    int GetExecutionMode() const override { return 0; }  // MODE_SEQUENTIAL
};

// ParallelAgent executes multiple sub-agents concurrently
class ParallelAgent : public WorkflowAgent {
public:
    ParallelAgent();
    explicit ParallelAgent(const std::string& name, const std::string& description = "");
    virtual ~ParallelAgent() = default;

    std::shared_ptr<AsyncIterator<std::shared_ptr<AgentEvent>>> Run(
        void* ctx,
        const std::shared_ptr<AgentInput>& input,
        const std::vector<std::shared_ptr<AgentRunOption>>& options) override;

    std::shared_ptr<AsyncIterator<std::shared_ptr<AgentEvent>>> Resume(
        void* ctx,
        const std::shared_ptr<ResumeInfo>& info,
        const std::vector<std::shared_ptr<AgentRunOption>>& options) override;

    int GetExecutionMode() const override { return 1; }  // MODE_PARALLEL
};

// LoopAgent repeatedly executes sub-agents
class LoopAgent : public WorkflowAgent {
public:
    LoopAgent();
    explicit LoopAgent(const std::string& name, const std::string& description = "", int max_iterations = 0);
    virtual ~LoopAgent() = default;

    std::shared_ptr<AsyncIterator<std::shared_ptr<AgentEvent>>> Run(
        void* ctx,
        const std::shared_ptr<AgentInput>& input,
        const std::vector<std::shared_ptr<AgentRunOption>>& options) override;

    std::shared_ptr<AsyncIterator<std::shared_ptr<AgentEvent>>> Resume(
        void* ctx,
        const std::shared_ptr<ResumeInfo>& info,
        const std::vector<std::shared_ptr<AgentRunOption>>& options) override;

    int GetExecutionMode() const override { return 2; }  // MODE_LOOP
    int GetMaxIterations() const override { return max_iterations_; }
    
    void SetMaxIterations(int max_iterations) { max_iterations_ = max_iterations; }

private:
    int max_iterations_;
};

// Configuration structs for factory functions
struct SequentialAgentConfig {
    std::string name;
    std::string description;
    std::vector<std::shared_ptr<Agent>> sub_agents;
};

struct ParallelAgentConfig {
    std::string name;
    std::string description;
    std::vector<std::shared_ptr<Agent>> sub_agents;
};

struct LoopAgentConfig {
    std::string name;
    std::string description;
    std::vector<std::shared_ptr<Agent>> sub_agents;
    int max_iterations = 0;  // 0 = unlimited
};

// Factory functions
std::shared_ptr<SequentialAgent> NewSequentialAgent(const SequentialAgentConfig& config);
std::shared_ptr<ParallelAgent> NewParallelAgent(const ParallelAgentConfig& config);
std::shared_ptr<LoopAgent> NewLoopAgent(const LoopAgentConfig& config);

}  // namespace adk
}  // namespace eino

#endif  // EINO_CPP_ADK_WORKFLOW_H_
