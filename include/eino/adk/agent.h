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

#ifndef EINO_CPP_ADK_AGENT_H_
#define EINO_CPP_ADK_AGENT_H_

#include "types.h"
#include "async_iterator.h"
#include "call_options.h"
#include <memory>
#include <string>
#include <vector>
#include <functional>

namespace eino {
namespace adk {

// Base Agent interface
class Agent {
public:
    virtual ~Agent() = default;

    // Name returns the agent name
    virtual std::string Name(void* ctx) = 0;

    // Description returns the agent description
    virtual std::string Description(void* ctx) = 0;

    // Run executes the agent with given input
    // Returns AsyncIterator for receiving AgentEvent results
    virtual std::shared_ptr<AsyncIterator<std::shared_ptr<AgentEvent>>> Run(
        void* ctx,
        const std::shared_ptr<AgentInput>& input,
        const std::vector<std::shared_ptr<AgentRunOption>>& options) = 0;
};

// Agents that support resumption after interruption
class ResumableAgent : public Agent {
public:
    virtual ~ResumableAgent() = default;

    // Resume resumes agent execution from interrupt point
    virtual std::shared_ptr<AsyncIterator<std::shared_ptr<AgentEvent>>> Resume(
        void* ctx,
        const std::shared_ptr<ResumeInfo>& info,
        const std::vector<std::shared_ptr<AgentRunOption>>& options) = 0;
};

// OnSubAgents interface for managing sub-agents
class OnSubAgents {
public:
    virtual ~OnSubAgents() = default;

    virtual void OnSetSubAgents(void* ctx, const std::vector<std::shared_ptr<Agent>>& agents) = 0;
    virtual void OnSetAsSubAgent(void* ctx, const std::shared_ptr<Agent>& parent) = 0;
    virtual void OnDisallowTransferToParent(void* ctx) = 0;
};

// AgentMiddleware for customizing agent behavior
struct AgentMiddleware {
    std::string additional_instruction;
    std::vector<void*> additional_tools;  // BaseTool pointers

    // Hooks for agent execution
    typedef std::function<void(void*, ChatModelAgentState*)> StateHandler;
    StateHandler before_chat_model;
    StateHandler after_chat_model;

    void* wrap_tool_call = nullptr;  // ToolMiddleware pointer
};

}  // namespace adk
}  // namespace eino

#endif  // EINO_CPP_ADK_AGENT_H_
