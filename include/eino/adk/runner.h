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

#ifndef EINO_CPP_ADK_RUNNER_H_
#define EINO_CPP_ADK_RUNNER_H_

// Runner - High-level Agent Execution Controller
// ===============================================
// Runner wraps an Agent and provides:
// - Simplified execution interface (Run, Query, Resume)
// - Checkpoint/interrupt persistence via CheckPointStore
// - Session value management  
// - Message input handling
//
// ARCHITECTURE (Aligns with eino adk.Runner):
// Runner acts as a facade over an Agent:
// - Initializes execution context
// - Handles message conversion to AgentInput
// - Manages checkpoint save/restore for interrupt/resume
// - Streams events from agent execution
// - Provides convenient string-based Query interface
//
// Design Pattern:
// User -> Runner.Run(messages) -> Agent.Run(input) -> Stream[AgentEvent]
//
// Aligns with eino adk.Runner structure and behavior

#include "types.h"
#include "async_iterator.h"
#include "agent.h"
#include "call_options.h"
#include "../compose/state.h"
#include <memory>
#include <string>
#include <vector>
#include <map>

namespace eino {
namespace adk {

// CheckPointStore interface for persisting interrupt state
class CheckPointStore {
public:
    virtual ~CheckPointStore() = default;

    // Save a checkpoint
    virtual void Save(const std::string& checkpoint_id,
                     const std::string& data) = 0;

    // Load a checkpoint
    virtual std::string Load(const std::string& checkpoint_id, bool& exists) = 0;

    // Check if checkpoint exists
    virtual bool Exists(const std::string& checkpoint_id) = 0;

    // Delete a checkpoint
    virtual void Delete(const std::string& checkpoint_id) = 0;
};

// Runner configuration
struct RunnerConfig {
    // The agent to run
    std::shared_ptr<Agent> agent;

    // Enable streaming output
    bool enable_streaming = false;

    // Optional checkpoint store for interrupt/resume functionality
    std::shared_ptr<CheckPointStore> checkpoint_store;

    RunnerConfig() = default;
    explicit RunnerConfig(std::shared_ptr<Agent> a) : agent(a) {}
};

// Runner manages high-level agent execution
class Runner {
public:
    explicit Runner(const RunnerConfig& config);
    explicit Runner(std::shared_ptr<Agent> agent);
    virtual ~Runner() = default;

    // Run executes the agent with given messages
    std::shared_ptr<AsyncIterator<std::shared_ptr<AgentEvent>>> Run(
        void* ctx,
        const std::vector<Message>& messages,
        const std::vector<std::shared_ptr<AgentRunOption>>& options = {});

    // Query runs the agent with a single query string
    std::shared_ptr<AsyncIterator<std::shared_ptr<AgentEvent>>> Query(
        void* ctx,
        const std::string& query,
        const std::vector<std::shared_ptr<AgentRunOption>>& options = {});

    // Resume resumes execution from a checkpoint
    // Returns error if checkpoint doesn't exist or loading fails
    std::pair<std::shared_ptr<AsyncIterator<std::shared_ptr<AgentEvent>>>, std::string> Resume(
        void* ctx,
        const std::string& checkpoint_id,
        const std::vector<std::shared_ptr<AgentRunOption>>& options = {});

    // SetCheckPointStore sets the checkpoint store
    void SetCheckPointStore(std::shared_ptr<CheckPointStore> store);

    // SetEnableStreaming sets whether streaming is enabled
    void SetEnableStreaming(bool enable);

    // GetAgent returns the underlying agent
    std::shared_ptr<Agent> GetAgent() const;

private:
    std::shared_ptr<Agent> agent_;
    bool enable_streaming_ = false;
    std::shared_ptr<CheckPointStore> checkpoint_store_;

    // Handle iterator events and manage checkpoints
    void HandleIteratorWithCheckpoint(
        void* ctx,
        std::shared_ptr<AsyncIterator<std::shared_ptr<AgentEvent>>> agent_iter,
        std::shared_ptr<AsyncGenerator<std::shared_ptr<AgentEvent>>> gen,
        const std::string& checkpoint_id);
};

// Factory function
std::shared_ptr<Runner> NewRunner(const RunnerConfig& config);
std::shared_ptr<Runner> NewRunner(std::shared_ptr<Agent> agent);

}  // namespace adk
}  // namespace eino

#endif  // EINO_CPP_ADK_RUNNER_H_
